#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

struct circular_buffer
{
   int		_beginIndex;
   int		_endIndex;
   int		_size;
   int		_capacity;
   char*	_data;
};

struct circular_buffer* circular_buffer_init(const int capacity);

void circular_buffer_term(struct circular_buffer* buf);

const int circular_buffer_write(struct circular_buffer* buf, const char* data, const int length);

const int circular_buffer_write_user(struct circular_buffer* buf, const char* data, const int length);

const int circular_buffer_read(struct circular_buffer* buf, char* data, const int length);

const int circular_buffer_read_user(struct circular_buffer* buf, char* data, const int length);

const int circular_buffer_num_bytes_available(struct circular_buffer* buf);

void circular_buffer_reset(struct circular_buffer* buf);

void circular_buffer_print_state(struct circular_buffer* buf);

#endif
