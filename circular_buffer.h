#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct message message;

/// Opaque circular buffer structure
typedef struct circular_buf_t circular_buf_t;

/// Handle type, the way users interact with the API
typedef circular_buf_t* cbuf_handle_t;

/// Pass in a storage buffer and size, returns a circular buffer handle
/// Requires: buffer is not NULL, size > 0
/// Ensures: cbuf has been created and is returned in an empty state
cbuf_handle_t circular_buf_init(message* buffer, size_t size);

/// Free a circular buffer structure
/// Requires: cbuf is valid and created by circular_buf_init
/// Does not free data buffer; owner is responsible for that
void circular_buf_free(cbuf_handle_t cbuf);

/// Reset the circular buffer to empty, head == tail. Data not cleared
/// Requires: cbuf is valid and created by circular_buf_init
void circular_buf_reset(cbuf_handle_t cbuf);

/// Get the possition of the oldest element in the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the possition of the oldest element in the buffer
size_t circular_buf_get_tail(cbuf_handle_t cbuf);

/// Get the possition of the newest element in the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the possition of the newest element in the buffer
size_t circular_buf_get_head(cbuf_handle_t cbuf);

/// Get the possition of the next value
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the position of the next value of the buffer
size_t circular_buf_next_pos(cbuf_handle_t cbuf, size_t current_pos);

/// Put version 1 continues to add data if the buffer is full
/// Old data is overwritten
/// Requires: cbuf is valid and created by circular_buf_init
void circular_buf_push(cbuf_handle_t cbuf, message data);

/// Retrieve a value from the buffer, the value is removed from the buffer and
///   the tail is advanced.
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns 0 on success, -1 if the buffer is empty
int circular_buf_pop(cbuf_handle_t cbuf, message * data);

/// Read the value from the buffer writen in the given position, the value
///   is NOT removed from the buffer.
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns 0 on success, -1 if the buffer is empty
int circular_buf_read(cbuf_handle_t cbuf, size_t position, message * data);

/// CHecks if the buffer is empty
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns true if the buffer is empty
bool circular_buf_empty(cbuf_handle_t cbuf);

/// Checks if the buffer is full
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns true if the buffer is full
bool circular_buf_full(cbuf_handle_t cbuf);

/// Check the capacity of the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the maximum capacity of the buffer
size_t circular_buf_capacity(cbuf_handle_t cbuf);

/// Check the number of elements stored in the buffer
/// Requires: cbuf is valid and created by circular_buf_init
/// Returns the current number of elements in the buffer
size_t circular_buf_size(cbuf_handle_t cbuf);

/// Search if the string is stored in the buffer
/// Requires: cbuf is valid and created by circular_buf_init
///           *msg is NOT NULL or empty.
/// Returns the possition of the string on success, -1 if the string is NOT found
size_t circular_buffer_find_msg(cbuf_handle_t cbuf, char *msg);

//TODO: int circular_buf_pop_range(circular_buf_t cbuf, message *data, size_t len);
//TODO: int circular_buf_push_range(circular_buf_t cbuf, message * data, size_t len);

#endif //CIRCULAR_BUFFER_H_
