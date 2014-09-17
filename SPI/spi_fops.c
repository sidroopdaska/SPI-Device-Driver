#include "spi_fops.h"
#include "spi_protocol.h"
#include "spi4.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#define __NO_VERSION_

extern struct spimod_device_state device_state;
extern struct spimod_transaction device_transaction;

long spimod_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param)
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
         tempUI2 = (device_transaction._inPacket->_status == SLAVE_RX_ABLE) ? 1: 0;

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

ssize_t spimod_read(struct file* file, char __user* buf, size_t count, loff_t* offp)
{
   printk(KERN_ALERT "spimod_read() - not implemented\n");

   return 0;
}

ssize_t spimod_write(struct file* file, const char __user* buf, size_t count, loff_t* offp)
{
   printk(KERN_ALERT "spimod_write() - not implemented\n");

   return 0;
}

int spimod_open(struct inode* i, struct file* file)
{
   int status = 0;

   if (down_interruptible(&device_state._fop_sem))
   {
      return -ERESTARTSYS;
   }

   if (!device_state._timer_running)
   {
      hrtimer_start(&device_state._timer,
                    ktime_set(device_state._timer_period_s, device_state._timer_period_ns),
                    HRTIMER_MODE_REL);

      device_state._timer_running = 1;
   }

   up(&device_state._fop_sem);

   return status;
}

int spimod_close(struct inode* i, struct file* file)
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

   circular_buffer_reset(device_state._txBuffer);
   circular_buffer_reset(device_state._rxBuffer);

   up(&device_state._fop_sem);
   
   return status;
}
