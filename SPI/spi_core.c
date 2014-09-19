/******************************************************************************
 *
 * Linux SPI Device Driver
 * (c) 2014 Avalon Sciences Ltd
 *
 * Module Name: spi_core
 *
 * Purpose:     Core functionality of the SPI device driver.  Handles module
 *              initialisation and termination plus the read / write timer.
 *
 * ***************************************************************************/

#include "spi4.h"
#include "spi_protocol.h"
#include "spi_fops.h"
#include "circular_buffer.h"

#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

/* Constants */

static const int TX_BUFFER_SIZE = 1024 * 16;
static const int RX_BUFFER_SIZE = 1024 * 64;

/* Global variables, used here and in other modules */

struct spimod_device_state device_state;
struct spimod_transaction device_transaction;

/* Externs, declared in spi_x.c */

extern const int SPI_BUS;
extern const char this_driver_name[];
extern const int makedev_id;

/* Instance of the driver handler */

static struct spi_driver spimod_driver = {
   .driver = {
           .name = this_driver_name,
           .owner = THIS_MODULE,
   },

   .probe	= spimod_probe,
   .remove	= __devexit_p(spimod_remove),
};

/* The file operations this driver is handling */

static const struct file_operations spimod_fops = {
   .owner		= THIS_MODULE,
   .read		= spimod_read,
   .write		= spimod_write,
   .unlocked_ioctl	= spimod_ioctl,
   .open		= spimod_open,
   .release		= spimod_close,
};

/******************************************************************************
 *
 * Function: spimod_timer_callback()
 * Purpose:  Timer callback that performs the read / write transaction on the
 *           SPI device.  It will not perform a read / write transaction if
 *           the previous one is still in progress.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: timer (timer context - not used).
 *
 * Returns:  Always HRTIMER_RESTART (to restart the timer).
 *
 * Globals:
 *
 * - device_state._timer_running (sanity check).
 * - device_transaction._busy (checks to see if a read / write transaction is
 *   already in progress).
 * - device_state._timer (the timer to use).
 *
 * ***************************************************************************/

static enum hrtimer_restart spimod_timer_callback(struct hrtimer* timer)
{
   if (device_state._timer_running && !device_transaction._busy)
   {
      spimod_create_outbound_packet();

      spimod_queue_spi_read_write();

      spimod_process_inbound_packet();
   }

   hrtimer_forward_now(
      &device_state._timer,
      ktime_set(device_state._timer_period_s, device_state._timer_period_ns));

   return HRTIMER_RESTART;
};

/******************************************************************************
 *
 * Function: spimod_init_spi()
 * Purpose:  Registers the SPI driver and adds it to the relevant bus.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, -1 on failure.
 *
 * Globals:
 *
 * - spimod_driver (the global driver structure).
 *
 * ***************************************************************************/

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

/******************************************************************************
 *
 * Function: spimod_init_cdev()
 * Purpose:  Initialises the character device and registers its declared
 *           file operations.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, -1 on failure.
 *
 * Globals:
 *
 * - device_state._devt (updated with the device handle).
 * - device_state._cdev (update with the character device handle).
 * - spimod_fops (the declared list of file operations).
 *
 * ***************************************************************************/

static int __init spimod_init_cdev(void)
{
   int error;

   device_state._devt = MKDEV(247, makedev_id);

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

/******************************************************************************
 *
 * Function: spimod_init_class()
 * Purpose:  Initialises the class of the character device.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, -1 on failure.
 *
 * Globals:
 *
 * - device_state._class (updated with the class handle).
 * - device_state._devt (the device handle to use).
 *
 * ***************************************************************************/

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

/******************************************************************************
 *
 * Function: spimod_init()
 * Purpose:  Module constructor - called once when the module is loaded (i.e.
 *           with insmod).
 *
 *           Registered with module_init() - see below.
 *
 *           The character device, the device class and the SPI driver are
 *           all initialised.  The outbound and inbound packets along with
 *           the transmit and receive circular buffers are also created.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  0 on success, -1 on failure.
 *
 * Globals:
 *
 * - device_state (defaulted)
 * - device_transaction (defaulted)
 * - device_transaction._outPacket (created / defaulted)
 * - device_transaction._inPacket (created / defaulted)
 * - device_state._timer (initialised)
 * - device_state._txBuffer (created)
 * - device_state._rxBuffer (created)
 *
 * ***************************************************************************/

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

   printk(KERN_ALERT "Module initialised\n");

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

/******************************************************************************
 *
 * Function: spimod_exit()
 * Purpose:  Module destructor - called once when the module is unloaded (i.e.
 *           with rmmod).
 *
 *           Registered with module_exit() - see below.
 *
 *           The character device, the device class and the SPI driver are
 *           all unregistered.  The outbound and inbound packets along with
 *           the transmit and receive circular buffers are also destroyed.
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
 * - device_state._spi_device (unregistered)
 * - spimod_driver (unregistered)
 * - device_state._class (destroyed)
 * - device_state._cdev (destroyed)
 * - device_state._devt (unregistered)
 * - device_state._txBuffer (destroyed)
 * - device_state._rxBuffer (destroyed)
 * - device_transaction._outPacket (destroyed)
 * - device_transaction._inPacket (destroyed)
 *
 * ***************************************************************************/

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

/* Registers the module initialiser and termination functions */

module_init(spimod_init);
module_exit(spimod_exit);

MODULE_AUTHOR("Siddarth Sharma & Steve Turnbull");
MODULE_DESCRIPTION("SPI Protocol Driver - PACKETS");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.4");
