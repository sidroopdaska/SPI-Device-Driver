/******************************************************************************
 *
 * Linux SPI Device Driver
 *
 * Module Name: spi_protocol
 *
 * Purpose:     Module containing functions relating to SPI bus interactions.
 *
 * ***************************************************************************/

#include "spi_protocol.h"
#include "circular_buffer.h"

#include <linux/module.h>
#include <linux/kernel.h>

#define __NO_VERSION_

/* Externs, declared in spi_core.c */

extern struct spimod_device_state device_state;
extern struct spimod_transaction device_transaction;

/* Externs, declared in spi_x.c */

extern const int SPI_BUS;
extern const char this_driver_name[];

/******************************************************************************
 *
 * Function: spimod_probe()
 * Purpose:  Probe callback function used to install the device in the driver
 *           at driver initialisation.
 *
 * Parameters:
 *
 * - IN:     spi_device (the device to install).
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  Always 0.
 *
 * Globals:
 *
 * - device_state._spi_device (set to the supplied device).
 *
 * ***************************************************************************/

int spimod_probe(
   struct spi_device* spi_device)
{
   unsigned long flags;

   spin_lock_irqsave(&device_state._spi_lock, flags);

   device_state._spi_device = spi_device;

   spin_unlock_irqrestore(&device_state._spi_lock, flags);

   return 0;
}

/******************************************************************************
 *
 * Function: spimod_remove()
 * Purpose:  Callback function for removal of the device at driver termination.
 *
 * Parameters:
 *
 * - IN:     spi_device (the current device) - not used.
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  Always 0.
 *
 * Globals:
 *
 * - device_state._spi_device (set to NULL).
 *
 * ***************************************************************************/

int spimod_remove(
   struct spi_device* spi_device)
{
   unsigned long flags;

   if (device_state._timer_running)
   {
      hrtimer_cancel(&device_state._timer);

      device_state._timer_running = 0;
   }

   spin_lock_irqsave(&device_state._spi_lock, flags);

   device_state._spi_device = NULL;

   spin_unlock_irqrestore(&device_state._spi_lock, flags);

   return 0;
}

/******************************************************************************
 *
 * Function: add_spimod_device_to_bus()
 * Purpose:  Called at driver initialisation to find a free SPI device.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, -1 on error.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

int add_spimod_device_to_bus(void)
{
   struct spi_master *spi_master;
   struct spi_device *spi_device;
   struct device *pdev;
   char buff[64];
   int status = 0;

   spi_master = spi_busnum_to_master(SPI_BUS);

   if (!spi_master)
   {
      printk(KERN_ALERT "spi_busnum_to_master(%d) returned NULL\n", SPI_BUS);
      printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n");

      return -1;
   }

   spi_device = spi_alloc_device(spi_master);

   if (!spi_device)
   {
      put_device(&spi_master->dev);

      printk(KERN_ALERT "spi_alloc_device() failed\n");

      return -1;
   }

   spi_device->chip_select = SPI_BUS_CS1;

   /* Check whether this SPI bus.cs is already claimed */

   snprintf(buff,
            sizeof(buff),
            "%s.%u",
            dev_name(&spi_device->master->dev),
            spi_device->chip_select);

   pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);

   if (pdev)
   {
      // We are not going to use this spi_device, so free it

      spi_dev_put(spi_device);

      if (pdev->driver && pdev->driver->name &&
          strcmp(this_driver_name, pdev->driver->name))
      {
         printk(KERN_ALERT "Driver [%s] already registered for %s\n",
                pdev->driver->name, buff);

         status = -1;
      }
   }
   else
   {
      spi_device->max_speed_hz = SPI_BUS_SPEED;
      spi_device->mode = SPI_MODE_0;
      spi_device->bits_per_word = 8;
      spi_device->irq = -1;
      spi_device->controller_state = NULL;
      spi_device->controller_data = NULL;

      strlcpy(spi_device->modalias, this_driver_name, SPI_NAME_SIZE);

      status = spi_add_device(spi_device);

      if (status < 0)
      {
         spi_dev_put(spi_device);

         printk(KERN_ALERT "spi_add_device() failed: %d\n", status);
      }
   }

   put_device(&spi_master->dev);

   return status;
}

/******************************************************************************
 *
 * Function: spimod_completion_handler()
 * Purpose:  Callback function for when the read / write transaction completes.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: arg (context - not used).
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - device_transaction._busy (set to 0).
 *
 * ***************************************************************************/

static void spimod_completion_handler(
   void* arg)
{
   //printk(KERN_ALERT "spimod_completion_handler()\n");

   device_transaction._busy = 0;
}

/******************************************************************************
 *
 * Function: spimod_queue_spi_read_write()
 * Purpose:  Performs the read / write transaction on the SPI device.
 *
 *           Should not be called if device_transaction._busy == 1.
 *
 *           Registers a completion handler callback for when the transaction
 *           completes.
 *
 *           Note: device_state._spi_device must point at a valid SPI device.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, negative integer on error.
 *
 * Globals:
 *
 * - device_transaction._msg (initialised for the read / write).
 * - device_transaction._transfer (initialised for the read / write).
 * - device_transaction._transfer.tx_buf (set to point at the out packet).
 * - device_transaction._transfer.rx_buf (set to point at the in packet).
 * - device_transaction._busy (set to 1 on success).
 *
 * ***************************************************************************/

int spimod_queue_spi_read_write(void)
{
   int status = 0;
   unsigned long flags;

   spi_message_init(&device_transaction._msg);

   device_transaction._msg.complete = spimod_completion_handler;
   device_transaction._msg.context = NULL;

   device_transaction._transfer.tx_buf = device_transaction._outPacket;
   device_transaction._transfer.rx_buf = device_transaction._inPacket;

   device_transaction._transfer.len = PACKET_SIZE;

   spi_message_add_tail(&device_transaction._transfer, &device_transaction._msg);

   spin_lock_irqsave(&device_state._spi_lock, flags);

   if (device_state._spi_device != NULL)
   {
      status = spi_async(device_state._spi_device, &device_transaction._msg);
   }
   else
   {
      status = -ENODEV;
   }

   spin_unlock_irqrestore(&device_state._spi_lock, flags);

   if (0 == status)
   {
      device_transaction._busy = 1; 
   }
   else
   {
      printk(KERN_NOTICE "spimod_queue_spi_read_write() failed: %d\n", status);
   }

   return status;
}

/******************************************************************************
 *
 * Function: spimod_create_outbound_packet()
 * Purpose:  Initialises and populates the outbound packet with up to 
 *           PACKET_DATA_SIZE bytes from the transmit circular buffer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - device_state._txBuffer (used to populate the outgoing packet).
 * - device_transaction._outPacket (initialised and populated with data).
 *
 * ***************************************************************************/

void spimod_create_outbound_packet(void)
{
   /* Read data from the tx buffer into our outbound packet */

   int len = circular_buffer_num_bytes_available(device_state._txBuffer);

   if (len > PACKET_DATA_SIZE)
   {
      len = PACKET_DATA_SIZE;
   }

   //printk(KERN_ALERT "Got %d bytes from tx buffer\n", len);

   device_transaction._outPacket->_sync = PACKET_SYNC;
   device_transaction._outPacket->_status = SLAVE_RX_UNABLE;
   device_transaction._outPacket->_len = len;

   memset(device_transaction._outPacket->_data, 0, PACKET_DATA_SIZE);

   circular_buffer_read(device_state._txBuffer,
                        device_transaction._outPacket->_data,
                        len);
}

/******************************************************************************
 *
 * Function: spimod_process_inbound_packet()
 * Purpose:  Validates the received packet and adds its data (if any) into the
 *           receive circular buffer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - device_state._rxBuffer (expanded with data from the incoming packet).
 * - device_transaction._inPacket (validated and data extracted).
 *
 * ***************************************************************************/

void spimod_process_inbound_packet(void)
{
   //printk (KERN_ALERT "Received %d bytes!\n", device_transaction._inPacket->_len);

   if ((PACKET_SYNC == device_transaction._inPacket->_sync)
    && (device_transaction._inPacket->_len <= PACKET_DATA_SIZE))
   {
      int numWritten;

      numWritten = circular_buffer_write(device_state._rxBuffer,
                                         device_transaction._inPacket->_data,
                                         device_transaction._inPacket->_len);

      if (numWritten != device_transaction._inPacket->_len)
      {
         printk(KERN_ALERT "Rx buffer overflow - %d bytes\n",
                device_transaction._inPacket->_len);
      }
   }
}
