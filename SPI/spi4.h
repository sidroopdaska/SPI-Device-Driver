/******************************************************************************
 *
 * Linux SPI Device Driver
 *
 * Module Name: spi4
 *
 * Purpose:     Header file declaring the interface between user space and
 *              the kernel driver.
 *
 * ***************************************************************************/

#ifndef SPIMOD_4_H
#define SPIMOD_4_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define MAJOR_NUM	247

/* Structure defining data transfers used by the relevant ioctl() calls */

struct spi_ioc_transfer
{
   __u8*	_buf;
   __u16	_bufLen;
};

/* Structure defining the device status as published to user space */

struct spi_ioc_status
{
   __u32	_rxBytesAvailable;
   __u32	_clearToSend;
};

/* ioctl() constants */

#define IOCTL_SEND_DATA		_IOR(MAJOR_NUM, 0, void*)
#define IOCTL_RECEIVE_DATA	_IOR(MAJOR_NUM, 1, void*)
#define IOCTL_GET_STATUS	_IOR(MAJOR_NUM, 2, void*)

#endif
