// uvscpd - Minimalist VSCP Daemon
// Copyright (C) 2019 Maarten Zanders
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
