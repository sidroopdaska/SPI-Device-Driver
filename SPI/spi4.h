#ifndef SPIMOD_4_H
#define SPIMOD_4_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define MAJOR_NUM	247

struct spi_ioc_transfer
{
   __u8*	_buf;
   __u16	_bufLen;
};

struct spi_ioc_status
{
   __u32	_rxBytesAvailable;
   __u32	_clearToSend;
};

#define IOCTL_SEND_DATA		_IOR(MAJOR_NUM, 0, void*)
#define IOCTL_RECEIVE_DATA	_IOR(MAJOR_NUM, 1, void*)
#define IOCTL_GET_STATUS	_IOR(MAJOR_NUM, 2, void*)

#define DEVICE_FILE_NAME	"spimod"

#endif
