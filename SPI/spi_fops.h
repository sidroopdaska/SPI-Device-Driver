/******************************************************************************
 *
 * Linux SPI Device Driver
 * Module Name: spi_fops
 *
 * Purpose:     A module containing file operations published by this driver.
 *
 * ***************************************************************************/

#ifndef SPI_FOPS_H
#define SPI_FOPS_H

#include <linux/fs.h>

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
   unsigned long ioctl_param);

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
   loff_t* offp);

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
   loff_t* offp);

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
   struct file* file);

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
   struct file* file);

#endif
