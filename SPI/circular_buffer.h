/******************************************************************************
 *
 * Linux SPI Device Driver
 *
 * Module Name: circular_buffer
 *
 * Purpose:     Provides a simple circular buffer, initialised to _capacity
 *              bytes.  Note writes will fail if asked to add more than
 *              (_capacity - _size) bytes.
 *
 * ***************************************************************************/


#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

/* The circular buffer structure */

struct circular_buffer
{
   int		_beginIndex;
   int		_endIndex;
   int		_size;
   int		_capacity;
   char*	_data;
};

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
   const int capacity);

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
   struct circular_buffer* buf);

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
   const int length);

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
   const int length);

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
   const int length);

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
   const int length);

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
   struct circular_buffer* buf);

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
   struct circular_buffer* buf);

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
   struct circular_buffer* buf);

#endif
