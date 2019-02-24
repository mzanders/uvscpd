#include <stdlib.h>
#include <assert.h>
#include <malloc.h>

#include "vscp_buffer.h"

typedef struct vscp_buffer_ctx {
  vscp_msg_t *buffer;
  unsigned int wr;
  unsigned int rd;
  unsigned int size;
} vscp_buffer_ctx_t;

vscp_buffer_ctx_t *vscp_buffer_ctx_create(unsigned int size) {
  vscp_buffer_ctx_t *ctx = malloc(sizeof(vscp_buffer_ctx_t));
  if (ctx != NULL) {
    ctx->wr = 0;
    ctx->rd = 0;
    ctx->size = size + 1;
    ctx->buffer = calloc(size + 1, sizeof(vscp_msg_t));
  }
  return ctx;
}

void vscp_buffer_free(vscp_buffer_ctx_t *ctx) {
  assert(ctx != NULL);
  free(ctx->buffer);
  free(ctx);
}

unsigned int next(vscp_buffer_ctx_t *ctx, unsigned int current) {
  current++;
  if (current >= ctx->size)
    current = 0;
  return current;
}

void vscp_buffer_push(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg) {
  assert(ctx != NULL);
  vscp_msg_t discard;

  if (next(ctx, ctx->wr) == ctx->rd)
    vscp_buffer_pop(ctx, &discard);

  ctx->buffer[ctx->wr] = *msg;
  ctx->wr = next(ctx, ctx->wr);
}

int vscp_buffer_pop(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg) {
  assert(ctx != NULL);
  int rv;

  if (ctx->rd != ctx->wr) {
    *msg = ctx->buffer[ctx->rd];
    ctx->rd = next(ctx, ctx->rd);
    rv = 0;
  } else
    rv = -1;

  return rv;
}

unsigned int vscp_buffer_used(vscp_buffer_ctx_t *ctx) {
  assert(ctx != NULL);
  if (ctx->wr >= ctx->rd)
    return ctx->wr - ctx->rd;
  else
    return (ctx->wr + ctx->size) - ctx->rd;
}

int vscp_buffer_flush(vscp_buffer_ctx_t *ctx) {
  assert(ctx != NULL);
  ctx->rd = ctx->wr;
  return 0;
}
