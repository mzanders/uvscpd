#include "cmd_interpreter.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  int argc;
  char **argv;
} command_history_t;

typedef struct cmd_interpreter_ctx {
  char *linebuffer;
  char *linebuffer_backup;
  char *linebuffer_history;
  int to_lower;
  int num_commands;
  size_t max_line_length;
  char *delimiters;
  char *writepointer;
  const cmd_interpreter_cmd_list_t *cmd_list;
  int max_argc;
  char **argv;
  int history_disable;
} cmd_interpreter_ctx_t;

static void *my_calloc(size_t __nmemb, size_t __size) {
  void *rv;
  rv = calloc(__nmemb, __size);
  if (rv == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(-1);
  }
  return rv;
}

cmd_interpreter_ctx_t *
cmd_interpreter_ctx_create(const cmd_interpreter_cmd_list_t *cmd_list,
                           int num_commands, int max_argc, int to_lower,
                           size_t max_line_length, const char *delimiters) {
  int i;
  assert(cmd_list != NULL);
  assert(max_line_length > 1);
  assert(delimiters != NULL);
  assert(max_argc > 0);
  for (i = 0; i < num_commands; i++) {
    assert(cmd_list[i].callback != NULL);
    assert(cmd_list[i].cmd_string != NULL);
  }
  cmd_interpreter_ctx_t *ctx = my_calloc(1, sizeof(cmd_interpreter_ctx_t));
  ctx->cmd_list = cmd_list;
  ctx->linebuffer = my_calloc(1, max_line_length + 1);
  ctx->linebuffer_history = my_calloc(1, max_line_length + 1);
  ctx->linebuffer_backup = my_calloc(1, max_line_length + 1);
  ctx->num_commands = num_commands;
  ctx->delimiters = strdup(delimiters);
  ctx->max_line_length = max_line_length;
  ctx->to_lower = to_lower;
  ctx->writepointer = ctx->linebuffer;
  ctx->max_argc = max_argc;
  ctx->argv = my_calloc(max_argc, sizeof(char *));
  return ctx;
}

void *cmd_interpreter_free(cmd_interpreter_ctx_t *ctx) {
  assert(ctx != NULL);
  free(ctx->argv);
  free(ctx->delimiters);
  free(ctx->linebuffer);
  free(ctx);
  return NULL;
}

static int cmd_interpreter_process_line(cmd_interpreter_ctx_t *ctx, char *line,
                                        void *obj) {
  char *saveptr;
  int rval;
  int i;
  ctx->argv[0] = strtok_r(line, ctx->delimiters, &saveptr);
  if (ctx->argv[0] == NULL) {
    rval = CMD_INTERPRETER_EMPTY_INPUT;
  } else {

    for (i = 0; i < ctx->num_commands; i++) {
      if (strcmp(ctx->cmd_list[i].cmd_string, ctx->argv[0]) == 0)
        break;
    }
    if (i == ctx->num_commands)
      rval = CMD_INTERPRETER_INVALID_COMMAND;
    else {
      int j;
      for (j = 1; j < ctx->max_argc; j++) {
        ctx->argv[j] = strtok_r(NULL, ctx->delimiters, &saveptr);
        if (ctx->argv[j] == NULL)
          break;
      }
      rval = ctx->cmd_list[i].callback(obj, j, ctx->argv);
    }
  }
  return rval;
}

int cmd_interpreter_repeat(cmd_interpreter_ctx_t *ctx, void *obj) {
  return cmd_interpreter_process_line(ctx, ctx->linebuffer_history, obj);
}

void cmd_interpreter_disable_history(cmd_interpreter_ctx_t *ctx) {
  ctx->history_disable = 1;
}

int cmd_interpreter_process(cmd_interpreter_ctx_t *ctx, char **buffer_start,
                            size_t buffer_len, void *obj) {
  int i;
  if (buffer_len == 0)
    return CMD_INTERPRETER_NO_MORE_DATA; /* nothing to do */
  /* copy up to the newline into our own buffer */
  int got_line = 0;
  for (i = 0; i < buffer_len; i++) {
    if (**buffer_start == '\n') {
      (*buffer_start)++;
      *(ctx->writepointer) = 0; /* NULL terminate */
      got_line = 1;
      break;
    } else {
      if ((**buffer_start >= ' ') /* only copy printable chars */
          && ((ctx->writepointer - ctx->linebuffer) < ctx->max_line_length)) {
        if (ctx->to_lower)
          *(ctx->writepointer) = tolower(**buffer_start);
        else
          *(ctx->writepointer) = **buffer_start;
        ctx->writepointer++;
      }
      (*buffer_start)++;
    }
  }
  if (!got_line)
    return CMD_INTERPRETER_NO_MORE_DATA; /* thank you, come again */
  strncpy(ctx->linebuffer_backup, ctx->linebuffer, ctx->max_line_length);

  int rval;
  if ((ctx->writepointer - ctx->linebuffer) == ctx->max_line_length) {
    rval = CMD_INTERPRETER_LINE_LENGTH_EXCEEDED;
  } else {
    rval = cmd_interpreter_process_line(ctx, ctx->linebuffer, obj);
    if (!ctx->history_disable)
      strncpy(ctx->linebuffer_history, ctx->linebuffer_backup,
              ctx->max_line_length);
    ctx->history_disable = 0;
  }

  /* reset for the next run! */
  ctx->writepointer = ctx->linebuffer;
  return rval;
}
