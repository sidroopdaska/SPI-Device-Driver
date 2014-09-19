/******************************************************************************
 *
 * Linux SPI Device Driver
 * (c) 2014 Avalon Sciences Ltd
 *
 * Module Name: circular_buffer
 *
 * Purpose:     Provides a simple circular buffer, initialised to _capacity
 *              bytes.  Note writes will fail if asked to add more than
 *              (_capacity - _size) bytes.
 *
 * ***************************************************************************/

#include "circular_buffer.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define __NO_VERSION__

/******************************************************************************
 *
 * Function: circular_buffer_init()
 * Purpose:  Initialises a new circular buffer to capacity bytes.
 *
 * Parameters:
 *
 * - IN:     capacity (requested maximum size of circular buffer).
 * - OUT:    N/A
 * - IN/OUT: N/A
 *
 * Returns:  An initialised circular buffer, or NULL.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

struct circular_buffer* circular_buffer_init(
   const int capacity)
{
   struct circular_buffer* buf = NULL;
	
   if (capacity > 0)
   {
      buf = kmalloc(sizeof(struct circular_buffer), GFP_KERNEL | GFP_DMA);

      buf->_size	= 0;
      buf->_beginIndex	= 0;
      buf->_endIndex	= 0;

      buf->_capacity	= capacity;
      buf->_data	= kmalloc(buf->_capacity, GFP_KERNEL | GFP_DMA);
   }

   return buf;
}

/******************************************************************************
 *
 * Function: circular_buffer_term()
 * Purpose:  Terminates a (non-NULL) circular buffer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: buf (the circular buffer to terminate).
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

void circular_buffer_term(
   struct circular_buffer* buf)
{
   if (buf != NULL)
   {
      kfree(buf->_data);
      kfree(buf);

      buf = NULL;
   }
}

/******************************************************************************
 *
 * Function: circular_buffer_write()
 * Purpose:  Writes bytes to the circular buffer.  Do not use with raw data
 *           from user space - use the equivalent _user version instead.
 *
 *           Always fails if length > (_capacity - _size).
 *
 * Parameters:
 *
 * - IN:     data (byte array).
 *           length (size of the byte array).
 * - OUT:    N/A
 * - IN/OUT: buf (the circular buffer to use)
 *
 * Returns:  The number of bytes written to the circular buffer.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

const int circular_buffer_write(
   struct circular_buffer* buf,
   const char* data,
   const int length)
{
   int result = 0;

   if (buf != NULL && data != NULL && length > 0)
   {
      int capacity = buf->_capacity;

      int bytesToWrite = min(length, buf->_capacity - buf->_size);

      if (bytesToWrite >= length)
      {
         if (bytesToWrite <= capacity - buf->_endIndex)
         {
            // Write in a single step...

            memcpy(buf->_data + buf->_endIndex,
                   data,
                   bytesToWrite);

            buf->_endIndex += bytesToWrite;

            if (buf->_endIndex == capacity)
            {
               buf->_endIndex = 0;
            }
         }
         else
         {
            // Write in two steps...

            int size1 = capacity - buf->_endIndex;
            int size2 = bytesToWrite - size1;

            memcpy(buf->_data + buf->_endIndex,
                   data,
                   size1);

            memcpy(buf->_data,
                   data + size1,
                   size2);

            buf->_endIndex = size2;
         }

         buf->_size += bytesToWrite;

         result = bytesToWrite;
      }
   }

   return result;
}

/******************************************************************************
 *
 * Function: circular_buffer_write_user()
 * Purpose:  As circular_buffer_write() but used for adding data from user
 *           space.
 *
 * Parameters:
 *
 * - As for circular_buffer_write().
 *
 * Returns:  As for circular_buffer_write().
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

const int circular_buffer_write_user(
   struct circular_buffer* buf,
   const char* data,
   const int length)
{
   int result = 0;

   if (buf != NULL && data != NULL && length > 0)
   {
      int capacity = buf->_capacity;

      int bytesToWrite = min(length, buf->_capacity - buf->_size);

      if (bytesToWrite >= length)
      {
         if (bytesToWrite <= capacity - buf->_endIndex)
         {
            // Write in a single step...

            if (0 == copy_from_user(buf->_data + buf->_endIndex,
                                    data,
                                    bytesToWrite))
            {
               buf->_endIndex += bytesToWrite;

               if (buf->_endIndex == capacity)
               {
                  buf->_endIndex = 0;
               }
            }
            else
            {
               bytesToWrite = 0;
            }
         }
         else
         {
            // Write in two steps...

            int size1 = capacity - buf->_endIndex;
            int size2 = bytesToWrite - size1;

            result = copy_from_user(buf->_data + buf->_endIndex,
                                    data,
                                    size1);

            result += copy_from_user(buf->_data,
                                     data + size1,
                                     size2);

            if (0 == result)
            {
               buf->_endIndex = size2;
            }
            else
            {
               bytesToWrite = 0;
            }
         }

         buf->_size += bytesToWrite;

         result = bytesToWrite;
      }
   }

   return result;
}

/******************************************************************************
 *
 * Function: circular_buffer_read()
 * Purpose:  Reads bytes from the circular buffer.  Do not use to add bytes
 *           to an array in user space - use the equivalent _user version
 *           instead.
 *
 *           Returns <= length bytes depending on the circular buffer _size.
 *
 * Parameters:
 *
 * - IN:     length (size of the byte array).
 * - OUT:    data (byte array).
 * - IN/OUT: buf (the circular buffer to use).
 *
 * Returns:  The number of bytes read from the circular buffer.
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

const int circular_buffer_read(
   struct circular_buffer* buf,
   char* data,
   const int length)
{
   int result = 0;

   if (buf != NULL && data != NULL && length > 0)
   {
      int capacity = buf->_capacity;

      int bytesToRead = min(length, buf->_size);

      if (bytesToRead <= buf->_capacity - buf->_beginIndex)
      {
         // Read in a single step...

         memcpy(data,
                buf->_data + buf->_beginIndex,
                bytesToRead);

         buf->_beginIndex += bytesToRead;

         if (buf->_beginIndex == buf->_capacity)
         {
            buf->_beginIndex = 0;
         }
      }
      else
      {
         // Read in two steps...

         int size1 = capacity - buf->_beginIndex;
         int size2 = bytesToRead - size1;

         memcpy(data,
                buf->_data + buf->_beginIndex,
                size1);

         memcpy(data + size1,
                buf->_data,
                size2);

         buf->_beginIndex = size2;
      }

      buf->_size -= bytesToRead;

      result = bytesToRead;
   }

   return result;
}

/******************************************************************************
 *
 * Function: circular_buffer_read_user()
 * Purpose:  As circular_buffer_read() but used for adding data to user
 *           space.
 *
 * Parameters:
 *
 * - As for circular_buffer_read().
 *
 * Returns:  As for circular_buffer_read().
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

const int circular_buffer_read_user(
   struct circular_buffer* buf,
   char* data,
   const int length)
{
   int result = 0;

   if (buf != NULL && data != NULL && length > 0)
   {
      int capacity = buf->_capacity;

      int bytesToRead = min(length, buf->_size);

      if (bytesToRead <= buf->_capacity - buf->_beginIndex)
      {
         // Read in a single step...

         if (0 == copy_to_user(data,
                               buf->_data + buf->_beginIndex,
                               bytesToRead))
         {
            buf->_beginIndex += bytesToRead;

            if (buf->_beginIndex == buf->_capacity)
            {
               buf->_beginIndex = 0;
            }
         }
         else
         {
            bytesToRead = 0;
         }
      }
      else
      {
         // Read in two steps...

         int size1 = capacity - buf->_beginIndex;
         int size2 = bytesToRead - size1;

         result = copy_to_user(data,
                               buf->_data + buf->_beginIndex,
                               size1);

         result += copy_to_user(data + size1,
                                buf->_data,
                                size2);

         if (0 == result)
         {
            buf->_beginIndex = size2;
         }
         else
         {
            bytesToRead = 0;
         }
      }

      buf->_size -= bytesToRead;

      result = bytesToRead;
   }

   return result;
}

/******************************************************************************
 *
 * Function: circular_buffer_num_bytes_available()
 * Purpose:  Returns the number of bytes available for reading in the circular
 *           buffer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: buf (the circular buffer to use).
 *
 * Returns:  The number of bytes available for reading (_size_);
 *
 * Globals:
 *
 * - [VAR_1] [DESC]
 * - [VAR_2] [DESC]
 *
 * ***************************************************************************/

const int circular_buffer_num_bytes_available(
   struct circular_buffer* buf)
{
   int result = 0;

   if (buf != NULL)
   {
      result = buf->_size;
   }

   return result;
}

/******************************************************************************
 *
 * Function: circular_buffer_reset()
 * Purpose:  Resets a circular buffer back to its initial state (_size == 0).
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: buf (the circular buffer to use).
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

void circular_buffer_reset(
   struct circular_buffer* buf)
{
   if (buf != NULL)
   {
      buf->_size	= 0;
      buf->_beginIndex	= 0;
      buf->_endIndex	= 0;
   }
}

/******************************************************************************
 *
 * Function: circular_buffer_print_state()
 * Purpose:  Prints the state of the circular buffer.
 *
 * Parameters:
 *
 * - IN:     N/A
 * - OUT:    N/A
 * - IN/OUT: buf (the circular buffer to use).
 *
 * Returns:  N/A
 *
 * Globals:
 *
 * - N/A
 *
 * ***************************************************************************/

void circular_buffer_print_state(
   struct circular_buffer* buf)
{
   if (buf != NULL)
   {
      printk(KERN_ALERT "Circular buffer: capacity = %d size = %d begin_index = %d end_index = %d\n",
             buf->_capacity,
             buf->_size,
             buf->_beginIndex,
             buf->_endIndex);
   }
}
