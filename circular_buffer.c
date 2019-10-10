#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "circular_buffer.h"

struct message {
	char msg[276]; // uint32_t + uint32_t + uint64_t + char[256] + 3 * "_" = 276 bytes
	uint64_t timestamp;
};

// The definition of our circular buffer structure is hidden from the user
struct circular_buf_t {
	message * buffer;
	size_t head;
	size_t tail;
	size_t max; //of the buffer
	bool empty;
	bool full;
};

#pragma mark - Private Functions -

static void advance_pointer(cbuf_handle_t cbuf)
{
	assert(cbuf);

	if(cbuf->full)
  {
      cbuf->tail = (cbuf->tail + 1) % cbuf->max;
  }

	cbuf->head = (cbuf->head + 1) % cbuf->max;

	// We mark full because we will advance tail on the next time around
	cbuf->full = ((cbuf->head + 1) % cbuf->max == cbuf->tail);
}

static void retreat_pointer(cbuf_handle_t cbuf)
{
	assert(cbuf);

	cbuf->full = false;
	if (cbuf->tail == cbuf->head) {
		cbuf->empty = true;
		return;
	}

	cbuf->tail = (cbuf->tail + 1) % cbuf->max;
}

#pragma mark - APIs -

cbuf_handle_t circular_buf_init(message* buffer, size_t size)
{
	assert(buffer && size);

	cbuf_handle_t cbuf = malloc(sizeof(circular_buf_t));
	assert(cbuf);

	cbuf->buffer = buffer;
	cbuf->max = size;
	circular_buf_reset(cbuf);

	assert(circular_buf_empty(cbuf));

	return cbuf;
}

void circular_buf_free(cbuf_handle_t cbuf)
{
	assert(cbuf);
	free(cbuf);
}

void circular_buf_reset(cbuf_handle_t cbuf)
{
    assert(cbuf);

    cbuf->head = 0;
    cbuf->tail = 0;
		cbuf->empty = true;
    cbuf->full = false;
}

size_t circular_buf_size(cbuf_handle_t cbuf)
{
	assert(cbuf);

	size_t size = cbuf->max;

	if(!cbuf->full)
	{
		if(cbuf->head >= cbuf->tail)
		{
			size = (cbuf->head - cbuf->tail) + 1;
		}
		else
		{
			size = (cbuf->max + cbuf->head - cbuf->tail) + 1;
		}

	}

	return size;
}

size_t circular_buf_capacity(cbuf_handle_t cbuf)
{
	assert(cbuf);

	return cbuf->max;
}

size_t circular_buf_get_tail(cbuf_handle_t cbuf)
{
	assert(cbuf);

	return cbuf->tail;
}

size_t circular_buf_get_head(cbuf_handle_t cbuf)
{
	assert(cbuf);

	return cbuf->head;
}

size_t circular_buf_next_pos(cbuf_handle_t cbuf, size_t current_pos)
{
	assert(cbuf);

  return (current_pos + 1) % cbuf->max;

}

void circular_buf_push(cbuf_handle_t cbuf, message data)
{
	assert(cbuf && cbuf->buffer);

	if(!cbuf->empty)
	{
		advance_pointer(cbuf);
	}

  cbuf->buffer[cbuf->head] = data;

	cbuf->empty = false;
}

int circular_buf_pop(cbuf_handle_t cbuf, message * data)
{
  assert(cbuf && data && cbuf->buffer);

  int r = -1;

  if(!circular_buf_empty(cbuf))
  {
    *data = cbuf->buffer[cbuf->tail];
    retreat_pointer(cbuf);

    r = 0;
  }

  return r;
}

int circular_buf_read(cbuf_handle_t cbuf, size_t position, message * data)
{
	assert(cbuf && data && cbuf->buffer);

  int r = -1;

  if(!circular_buf_empty(cbuf))
  {
    *data = cbuf->buffer[position];

    r = 0;
  }

  return r;
}

bool circular_buf_empty(cbuf_handle_t cbuf)
{
	assert(cbuf);

  return cbuf->empty;
}

bool circular_buf_full(cbuf_handle_t cbuf)
{
	assert(cbuf);

    return cbuf->full;
}

size_t circular_buffer_find_msg(cbuf_handle_t cbuf, char *msg)
{
	assert(cbuf && cbuf->buffer && msg);

	size_t r = -1;

	if (!circular_buf_empty(cbuf))
	{
		size_t cb_capacity = circular_buf_capacity(cbuf);
		size_t it, i;

		// scan buffer
		for(it = cbuf->tail, i = 0; i < cb_capacity;
									it = circular_buf_next_pos(cbuf, it), i++)
		{
			r = -2; // message not found

			if (strcmp(cbuf->buffer[it].msg, msg) == 0) 	// if string is found
			{
				r = it;		// save position
				break;
			}
		}
	}

	return r;
}
