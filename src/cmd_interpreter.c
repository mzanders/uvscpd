#include "cmd_interpreter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct cmd_interpreter_ctx {
  char* linebuffer;
  int case_sensitive;
  int num_commands;
  size_t max_line_length;
  char* delimiters;
  char* writepointer;
  const cmd_interpreter_cmd_list_t* cmd_list;
  char** argv;
} cmd_interpreter_ctx_t;

cmd_interpreter_ctx_t* cmd_interpreter_ctx_create(
    const cmd_interpreter_cmd_list_t* cmd_list, int num_commands,
    int case_sensitive, size_t max_line_length, const char* delimiters) {
  int i;
  int max_argc;

  assert(cmd_list != NULL);
  assert(max_line_length > 1);
  assert(delimiters != NULL);
  cmd_interpreter_ctx_t* ctx = malloc(sizeof(cmd_interpreter_ctx_t));
  if (ctx == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(-1);
  }

  ctx->cmd_list = cmd_list;
  ctx->linebuffer = malloc(max_line_length + 1);
  if (ctx->linebuffer == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(-1);
  }
  ctx->num_commands = num_commands;
  ctx->delimiters = strdup(delimiters);
  ctx->max_line_length = max_line_length;
  ctx->case_sensitive = case_sensitive;
  ctx->writepointer = ctx->linebuffer;

  max_argc = 0;
  for (i = 0; i < num_commands; i++) {
    if (cmd_list[i].argc > max_argc) max_argc = cmd_list[i].argc;
  }
  ctx->argv = calloc(sizeof(char*), max_argc);
  if (ctx->argv == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(-1);
  }

  return ctx;
}

void cmd_interpreter_free(cmd_interpreter_ctx_t* ctx) {
  assert(ctx != NULL);
  free(ctx->argv);
  free(ctx->delimiters);
  free(ctx->linebuffer);
  free(ctx);
}

int cmd_interpreter_process(cmd_interpreter_ctx_t* ctx, char** buffer_start,
                            size_t buffer_len, int* argc, char** argv[]) {
  int i;
  if (buffer_len == 0) return 0; /* nothing to do */
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
        *(ctx->writepointer) = **buffer_start;
        ctx->writepointer++;
      }
      (*buffer_start)++;
    }
  }
  if (!got_line) return 0; /* thank you, come again */

  if ((ctx->writepointer - ctx->linebuffer) == ctx->max_line_length) {
    return CMD_INTERPRETER_LINE_LENGTH_EXCEEDED;
  }
  /* process the line that's in our buffer */
  char* saveptr;
  char* char_command;
  int rval;
  char_command = strtok_r(ctx->linebuffer, ctx->delimiters, &saveptr);

  if (char_command == NULL) {
    rval = CMD_INTERPRETER_INVALID_INPUT;
  } else {
    for (i = 0; i < ctx->num_commands; i++) {
      if (strcmp(ctx->cmd_list[i].cmd_string, char_command) == 0) break;
    }
    if (i == ctx->num_commands)
      rval = CMD_INTERPRETER_INVALID_COMMAND;
    else {
      int j;
      for (j = 0; j < ctx->cmd_list[i].argc; j++) {
        ctx->argv[j] = strtok_r(NULL, ctx->delimiters, &saveptr);
        if (ctx->argv[j] == NULL) break;
      }
      if(strtok_r(NULL, ctx->delimiters, &saveptr) != NULL)
        j = -1;
      if(j == ctx->cmd_list[i].argc)
      {
        rval = ctx->cmd_list[i].cmd_id;
        *argc = ctx->cmd_list[i].argc;
        *argv = ctx->argv;
      }
      else
      {
        rval = CMD_INTERPRETER_INVALID_NUM_PARAMETERS;
      }
    }
  }
  /* reset for the next run! */
  ctx->writepointer = ctx->linebuffer;
  return rval;
}
