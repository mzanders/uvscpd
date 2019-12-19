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

#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <pthread.h>

#include "vscp_buffer.h"

typedef struct vscp_buffer_ctx {
  vscp_msg_t *buffer;
  unsigned int wr;
  unsigned int rd;
  unsigned int size;
  pthread_mutex_t mutex;
} vscp_buffer_ctx_t;

vscp_buffer_ctx_t *vscp_buffer_ctx_create(unsigned int size) {
  vscp_buffer_ctx_t *ctx = malloc(sizeof(vscp_buffer_ctx_t));
  if (ctx != NULL) {
    ctx->wr = 0;
    ctx->rd = 0;
    ctx->size = size + 1;
    ctx->buffer = calloc(size + 1, sizeof(vscp_msg_t));
    pthread_mutex_init(&(ctx->mutex), NULL);
  }
  return ctx;
}

void vscp_buffer_free(vscp_buffer_ctx_t *ctx) {
  assert(ctx != NULL);
  free(ctx->buffer);
  pthread_mutex_destroy(&(ctx->mutex));
  free(ctx);
}

static unsigned int next(vscp_buffer_ctx_t *ctx, unsigned int current) {
  current++;
  if (current >= ctx->size)
    current = 0;
  return current;
}

void vscp_buffer_push(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg) {
  assert(ctx != NULL);
  vscp_msg_t discard;

  pthread_mutex_lock(&(ctx->mutex));

  if (next(ctx, ctx->wr) == ctx->rd)
    vscp_buffer_pop(ctx, &discard);

  ctx->buffer[ctx->wr] = *msg;
  ctx->wr = next(ctx, ctx->wr);

  pthread_mutex_unlock(&(ctx->mutex));

}

int vscp_buffer_pop(vscp_buffer_ctx_t *ctx, vscp_msg_t *msg) {
  assert(ctx != NULL);
  int rv;

  pthread_mutex_lock(&(ctx->mutex));

  if (ctx->rd != ctx->wr) {
    *msg = ctx->buffer[ctx->rd];
    ctx->rd = next(ctx, ctx->rd);
    rv = 0;
  } else
    rv = -1;

  pthread_mutex_unlock(&(ctx->mutex));

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

  pthread_mutex_lock(&(ctx->mutex));

  ctx->rd = ctx->wr;

  pthread_mutex_unlock(&(ctx->mutex));

  return 0;
}
