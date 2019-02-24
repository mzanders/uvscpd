#ifndef _VSCP_BUFFER_H_
#define _VSCP_BUFFER_H_

#include "vscp.h"

typedef struct vscp_buffer_ctx vscp_buffer_ctx_t;

vscp_buffer_ctx_t *vscp_buffer_ctx_create(unsigned int size);

void vscp_buffer_free(vscp_buffer_ctx_t *ctx);

void vscp_buffer_push(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg);
int vscp_buffer_pop(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg);
unsigned int vscp_buffer_used(vscp_buffer_ctx_t *ctx);
int vscp_buffer_flush (vscp_buffer_ctx_t *ctx);

#endif /* _VSCP_BUFFER_H_ */
