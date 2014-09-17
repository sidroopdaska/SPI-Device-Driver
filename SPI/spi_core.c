#include "spi4.h"
#include "spi_protocol.h"
#include "spi_fops.h"
#include "circular_buffer.h"

#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

/******************************
 * Constants                  *
 *****************************/

static const int TX_BUFFER_SIZE = 1024 * 16;
static const int RX_BUFFER_SIZE = 1024 * 64;

/******************************
 * End of Constants           *
 *****************************/

/******************************
 * Global Variables           *
 *****************************/

struct spimod_device_state device_state;
struct spimod_transaction device_transaction;

static struct spi_driver spimod_driver = {
   .driver = {
           .name = this_driver_name,
           .owner = THIS_MODULE,
   },

   .probe	= spimod_probe,
   .remove	= __devexit_p(spimod_remove),
};

static const struct file_operations spimod_fops = {
   .owner		= THIS_MODULE,
   .read		= spimod_read,
   .write		= spimod_write,
   .unlocked_ioctl	= spimod_ioctl,
   .open		= spimod_open,
   .release		= spimod_close,
};

/******************************
 * End of Global Variables    *
 *****************************/

static enum hrtimer_restart spimod_timer_callback(struct hrtimer* timer)
{
   if (device_state._timer_running && !device_transaction._busy)
   {
      spimod_create_outbound_packet();

      spimod_queue_spi_read_write();

      spimod_process_inbound_packet();
   }

   hrtimer_forward_now(&device_state._timer,
                       ktime_set(device_state._timer_period_s, device_state._timer_period_ns));

   return HRTIMER_RESTART;
};

static int __init spimod_init_spi(void)
{
   int error = spi_register_driver(&spimod_driver);

   if (error < 0)
   {
      printk(KERN_ALERT "spi_register_driver() failed %d\n", error);

      return -1;
   }

   error = add_spimod_device_to_bus();

   if (error < 0)
   {
      printk(KERN_ALERT "add_spimod_to_bus() failed %d\n", error);

      spi_unregister_driver(&spimod_driver);

      return -1;
   }

   return 0;
};

static int __init spimod_init_cdev(void)
{
   int error;

   device_state._devt = MKDEV(247, 0);

   error = register_chrdev_region(device_state._devt, 1, this_driver_name);

   if (error < 0)
   {
      printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", error);

      return -1;
   }

   cdev_init(&device_state._cdev, &spimod_fops);

   device_state._cdev.owner = THIS_MODULE;

   error = cdev_add(&device_state._cdev, device_state._devt, 1);

   if (error)
   {
      printk(KERN_ALERT "cdev_add() failed: %d\n", error);

      unregister_chrdev_region(device_state._devt, 1);

      return -1;
   }

   return 0;
};

static int __init spimod_init_class(void)
{
   device_state._class = class_create(THIS_MODULE, this_driver_name);

   if (!device_state._class)
   {
      printk(KERN_ALERT "class_create() failed\n");

      return -1;
   }

   if (!device_create(device_state._class,
                      NULL,
                      device_state._devt,
                      NULL,
                      this_driver_name))
   {
      printk(KERN_ALERT "device_create(..., %s) failed\n", this_driver_name);

      class_destroy(device_state._class);

      return -1;
   }

   return 0;
};

static int __init spimod_init(void)
{
   printk(KERN_ALERT "Initialising module...\n");

   memset(&device_state, 0, sizeof(struct spimod_device_state));
   memset(&device_transaction, 0, sizeof(struct spimod_transaction));

   device_transaction._outPacket = kmalloc(PACKET_SIZE, GFP_KERNEL | GFP_DMA);

   memset(device_transaction._outPacket, 0, PACKET_SIZE);

   device_transaction._inPacket = kmalloc(PACKET_SIZE, GFP_KERNEL | GFP_DMA);

   memset(device_transaction._inPacket, 0, PACKET_SIZE);

   spin_lock_init(&device_state._spi_lock);

   sema_init(&device_state._fop_sem, 1);
   sema_init(&device_state._spi_sem, 1);

   if (spimod_init_cdev() < 0)
   {
      goto fail_1;
   }

   if (spimod_init_class() < 0)
   {
      goto fail_2;
   }

   if (spimod_init_spi() < 0)
   {
      goto fail_3;
   }

   // Adding timer info

   device_state._timer_period_s = 0;
   device_state._timer_period_ns = NANOSECS_PER_SEC / WRITE_FREQUENCY;

   hrtimer_init(&device_state._timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

   device_state._timer.function = spimod_timer_callback;

   device_state._txBuffer = circular_buffer_init(TX_BUFFER_SIZE);
   device_state._rxBuffer = circular_buffer_init(RX_BUFFER_SIZE);

   if ((NULL == device_state._txBuffer)
    || (NULL == device_state._rxBuffer))
   {
      printk(KERN_ALERT "circular_buffer_init() failed tx = %X rx = %X\n",
             (unsigned int)device_state._txBuffer,
             (unsigned int)device_state._rxBuffer);

      goto fail_3;
   }

   printk(KERN_ALERT "Module initialisation complete\n");

   return 0;

fail_3:
        device_destroy(device_state._class, device_state._devt);
        class_destroy(device_state._class);

fail_2:
        cdev_del(&device_state._cdev);
        unregister_chrdev_region(device_state._devt, 1);

fail_1:
        return -1;
}

static void __exit spimod_exit(void)
{
   printk(KERN_ALERT "Terminating module...\n");

   spi_unregister_device(device_state._spi_device);
   spi_unregister_driver(&spimod_driver);

   device_destroy(device_state._class, device_state._devt);
   class_destroy(device_state._class);

   cdev_del(&device_state._cdev);
   unregister_chrdev_region(device_state._devt, 1);

   circular_buffer_term(device_state._txBuffer);
   circular_buffer_term(device_state._rxBuffer);

   kfree(device_transaction._outPacket);
   kfree(device_transaction._inPacket);

   printk(KERN_ALERT "Module terminated\n");
};

module_init(spimod_init);
module_exit(spimod_exit);

MODULE_AUTHOR("Siddarth Sharma & Steve Turnbull");
MODULE_DESCRIPTION("SPI Protocol Driver - PACKETS");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.4");
