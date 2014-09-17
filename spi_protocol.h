#ifndef SPI_PROTOCOL_H
#define SPI_PROTOCOL_H

#include "circular_buffer.h"

#include <linux/spi/spi.h>
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/hrtimer.h>

#define PACKET_DATA_SIZE		1540

#pragma pack(1)

struct packet
{
   unsigned short		_sync;
   short               		_status;
   unsigned short		_len;
   unsigned char		_data[PACKET_DATA_SIZE];
};

#pragma pack()

struct spimod_transaction
{
   struct spi_message		_msg;
   struct spi_transfer		_transfer;
   struct packet		_outPacket;
   struct packet		_inPacket;
   u32				_busy;
};

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

typedef enum
{
   SLAVE_RX_UNABLE,
   SLAVE_RX_ABLE

} packetStatusType;

static const unsigned short PACKET_SYNC	= 0xA5A5;

static const unsigned int PACKET_SIZE   = sizeof(struct packet);

static const int USER_BUFF_SIZE		= 5000;

static const char this_driver_name[]	= "spimod";

static const int SPI_BUS		= 2;
static const int SPI_BUS_CS1		= 1;
static const int SPI_BUS_SPEED		= 4000000;

static const int TIMEOUT		= 250;
static const int WRITE_FREQUENCY	= 1000;
static const int NANOSECS_PER_SEC	= 1000000000;

int spimod_probe(struct spi_device* spi_device);

int spimod_remove(struct spi_device* spi_device);

int add_spimod_device_to_bus(void);

int spimod_queue_spi_read_write(void);

void spimod_create_outbound_packet(void);

void spimod_process_inbound_packet(void);

#endif
