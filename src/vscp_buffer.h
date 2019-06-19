#ifndef _VSCP_BUFFER_H_
#define _VSCP_BUFFER_H_

/* NON THREAD SAFE FIFO buffer for VSCP messages */

#include "vscp.h"

// Context to work with
typedef struct vscp_buffer_ctx vscp_buffer_ctx_t;

// Set up a context to hold 'size' messages
vscp_buffer_ctx_t *vscp_buffer_ctx_create(unsigned int size);

// Destroy/free the context
void vscp_buffer_free(vscp_buffer_ctx_t *ctx);

// Add the message to the buffer. If the buffer was full, the oldest message
// gets discarded silently.
void vscp_buffer_push(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg);

// Get the oldest message from the buffer. Returns 0 if there was one, -1 if the
// buffer was empty.
int vscp_buffer_pop(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg);

// Get the amount of messages in the buffer
unsigned int vscp_buffer_used(vscp_buffer_ctx_t *ctx);

// Empty the buffer
int vscp_buffer_flush (vscp_buffer_ctx_t *ctx);

#endif /* _VSCP_BUFFER_H_ */
