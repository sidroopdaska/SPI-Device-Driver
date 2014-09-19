/******************************************************************************
 *
 * Linux SPI Device Driver
 * (c) 2014 Avalon Sciences Ltd
 *
 * Module Name: spi_fops
 *
 * Purpose:     A module containing file operations published by this driver.
 *
 * ***************************************************************************/

#include "spi_fops.h"
#include "spi_protocol.h"
#include "spi4.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define __NO_VERSION_

/* Externs, declared in spi_core.c */

extern struct spimod_device_state device_state;
extern struct spimod_transaction device_transaction;

/******************************************************************************
 *
 * Function: spimod_ioctl()
 * Purpose:  Generic handler for ioctl() calls received by this driver.
 *
 * Parameters:
 *
 * - IN:     ioctl_num (the ioctl call id).
 * - OUT:    N/A
 * - IN/OUT: file (file pointer data - not used).
 *           ioctl_param (pointer to data specific to the ioctl id).
 *
 * Returns:  Specific to the ioctl id but >= 0 on success, negative integer
 *           on failure.
 *
 * Globals:
 *
 * - device_state._txBuffer (to add data from the user).
 * - device_state._rxBuffer (to extract data for the user).
 * - device_transaction._inPacket->_status (for slave CTS status).
 *
 * ***************************************************************************/

long spimod_ioctl(
   struct file* file,
   unsigned int ioctl_num,
   unsigned long ioctl_param)
{
   long result = 0;

   struct spi_ioc_transfer* data_params = NULL;
   struct spi_ioc_status* status_params = NULL;

   int numBytes;
   unsigned short tempUS1;
   unsigned int tempUI1, tempUI2;

   //printk(KERN_ALERT "spimod_ioctl()\n");

   if (down_interruptible(&device_state._fop_sem))
   {
      return -ERESTARTSYS;
   }

   switch (ioctl_num)
   {
      case IOCTL_SEND_DATA:

         //printk(KERN_ALERT "IOCTL_SEND_DATA\n");

         data_params = (struct spi_ioc_transfer*)ioctl_param;
 
         get_user(tempUS1, &data_params->_bufLen);

         numBytes = circular_buffer_write_user(device_state._txBuffer,
                                               data_params->_buf,
                                               tempUS1);

         if (numBytes != data_params->_bufLen)
         {
            printk(KERN_ALERT "IOCTL_SEND_DATA - requested %d written %d\n",
                   data_params->_bufLen,
                   numBytes);
         }

         result = numBytes;

         break;

      case IOCTL_RECEIVE_DATA:

         //printk(KERN_ALERT "IOCTL_RECEIVE_DATA\n");

         data_params = (struct spi_ioc_transfer*)ioctl_param;

         get_user(tempUS1, &data_params->_bufLen);

         numBytes = circular_buffer_read_user(device_state._rxBuffer,
                                              data_params->_buf,
                                              tempUS1);

         result = numBytes;

         break;

      case IOCTL_GET_STATUS:

         //printk(KERN_ALERT "IOCTL_GET_STATUS\n");

         status_params = (struct spi_ioc_status*)ioctl_param;

         tempUI1 = circular_buffer_num_bytes_available(device_state._rxBuffer);
         tempUI2 = (device_transaction._inPacket->_status == SLAVE_RX_ABLE)
		      ? 1: 0;

         put_user(tempUI1, &status_params->_rxBytesAvailable);
         put_user(tempUI2, &status_params->_clearToSend);

         result = 0;

         break;

      default:

         printk(KERN_ALERT "Unsupported ioctl\n");

         result = -1;

         break;
   }

   up(&device_state._fop_sem);

   return result;
}

/******************************************************************************
 *
 * Function: spimod_read()
 * Purpose:  Handler for the read() system call.
 *
 *           Not implemented.
 *
 * Parameters:
 *
 * - IN:     count (size of the user-supplied buffer).
 * - OUT:    N/A
 * - IN/OUT: file (file pointer data).
 *           buf(user-supplied buffer).
 *           offp (offset within the file).
 *
 * Returns:  Number of bytes read, negative integer on failure.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

ssize_t spimod_read(
   struct file* file,
   char __user* buf,
   size_t count,
   loff_t* offp)
{
   printk(KERN_ALERT "spimod_read() - not implemented\n");

   return 0;
}

/******************************************************************************
 *
 * Function: spimod_write()
 * Purpose:  Handler for the write() system call.
 *
 *           Not implemented.
 *
 * Parameters:
 *
 * - IN:     count (size of the user-supplied buffer).
 *           buf (user-supplied buffer).
 * - OUT:    N/A
 * - IN/OUT: file (file pointer data).
 *           offp (offset within the file).
 *
 * Returns:  Number of bytes written, negative integer on failure.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

ssize_t spimod_write(
   struct file* file,
   const char __user* buf,
   size_t count,
   loff_t* offp)
{
   printk(KERN_ALERT "spimod_write() - not implemented\n");

   return 0;
}

/******************************************************************************
 *
 * Function: spimod_open()
 * Purpose:  Handler for the open() system call.  Resets the transmit and
 *           receive circular buffers, clears the outbound and inbound packets
 *           and starts the read / write timer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: i (current file system inode - not used).
 *           file (file pointer data - not used).
 *
 * Returns:  0 on success, negative integer on failure.
 *
 * Globals:
 *
 * - device_state._txBuffer (cleared).
 * - device_state._rxBuffer (cleared).
 * - device_transaction._outPacket (cleared).
 * - device_transaction._inPacket (cleared).
 * - device_state._timer (started).
 *
 * ***************************************************************************/

int spimod_open(
   struct inode* i,
   struct file* file)
{
   int status = 0;

   if (down_interruptible(&device_state._fop_sem))
   {
      return -ERESTARTSYS;
   }

   circular_buffer_reset(device_state._txBuffer);
   circular_buffer_reset(device_state._rxBuffer);

   memset(device_transaction._outPacket, 0, PACKET_SIZE);
   memset(device_transaction._inPacket, 0, PACKET_SIZE);

   if (!device_state._timer_running)
   {
      hrtimer_start(
         &device_state._timer,
         ktime_set(device_state._timer_period_s, device_state._timer_period_ns),
         HRTIMER_MODE_REL);

      device_state._timer_running = 1;
   }

   up(&device_state._fop_sem);

   return status;
}

/******************************************************************************
 *
 * Function: spimod_close()
 * Purpose:  Handler for the close() system call.  Stops the read / write
 *           timer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: i (current file system inode - not used).
 *           file (file pointer data - not used).
 *
 * Returns:  Always 0.
 *
 * Globals:
 *
 * - device_state._timer (stopped).
 *
 * ***************************************************************************/

int spimod_close(
   struct inode* i,
   struct file* file)
{
   int status = 0;

   if (down_interruptible(&device_state._fop_sem))
   {
      return -ERESTARTSYS;
   }

   if (device_state._timer_running)
   {
      hrtimer_cancel(&device_state._timer);

      device_state._timer_running = 0;
   }

   up(&device_state._fop_sem);
   
   return status;
}
