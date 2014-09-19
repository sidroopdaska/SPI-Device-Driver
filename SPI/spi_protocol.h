/******************************************************************************
 *
 * Linux SPI Device Driver
 * (c) 2014 Avalon Sciences Ltd
 *
 * Module Name: spi_protocol
 *
 * Purpose:     Module containing functions relating to SPI bus interactions.
 *
 * ***************************************************************************/

#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#include "circular_buffer.h"

#include <linux/spi/spi.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/hrtimer.h>

#define PACKET_DATA_SIZE		1540

/* The packet used for the SPI transmit / receive interaction */

#pragma pack(1)

struct packet
{
   unsigned short		_sync;
   short               		_status;
   unsigned short		_len;
   unsigned char		_data[PACKET_DATA_SIZE];
};

#pragma pack()

/* The SPI transaction state */

struct spimod_transaction
{
   struct spi_message		_msg;
   struct spi_transfer		_transfer;
   struct packet*		_outPacket;
   struct packet*		_inPacket;
   u32				_busy;
};

/* The device driver state */

struct spimod_device_state
{
   spinlock_t			_spi_lock;
   struct semaphore		_fop_sem;
   struct semaphore		_spi_sem;
   dev_t			_devt;
   struct cdev			_cdev;
   struct class*		_class;
   struct spi_device*		_spi_device;
   // Timer
   struct hrtimer		_timer;
   u32				_timer_period_s;
   u32				_timer_period_ns;
   u32				_timer_running;
   // Buffers
   struct circular_buffer*      _txBuffer;
   struct circular_buffer*	_rxBuffer;
};

/* The SPI slave state */

typedef enum
{
   SLAVE_RX_UNABLE,
   SLAVE_RX_ABLE

} packetStatusType;

/* Constants */

static const unsigned short PACKET_SYNC	= 0xA5A5;

static const unsigned int PACKET_SIZE   = sizeof(struct packet);

static const int SPI_BUS_CS1		= 1;
static const int SPI_BUS_SPEED		= 4000000;

static const int WRITE_FREQUENCY	= 1000;
static const int NANOSECS_PER_SEC	= 1000000000;

/* End of Constants */

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
   struct spi_device* spi_device);

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
   struct spi_device* spi_device);

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

int add_spimod_device_to_bus(void);

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

int spimod_queue_spi_read_write(void);

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

void spimod_create_outbound_packet(void);

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

void spimod_process_inbound_packet(void);

#endif
