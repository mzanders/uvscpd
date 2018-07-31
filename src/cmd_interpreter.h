#ifndef _CMD_INTERPRETER_H_
#define _CMD_INTERPRETER_H_

#include <stdlib.h>

typedef struct cmd_interpreter_ctx cmd_interpreter_ctx_t;

typedef struct {
  int cmd_id;  // id returned when command has been decoded, must be > 0
  const char* cmd_string;  // ascii string for this command
} cmd_interpreter_cmd_list_t;

cmd_interpreter_ctx_t* cmd_interpreter_ctx_create(
    const cmd_interpreter_cmd_list_t* cmd_list,  // command list structure
    int num_commands,                            // number of elements in above
    int max_argc,            // max number of arguments that can be used
    int case_sensitive,                          // ignore casing on decoding
    size_t max_line_length,  // used for allocating internal buffer
    const char* delimiters   // list of delimiters between command & arguments
);

void * cmd_interpreter_free(cmd_interpreter_ctx_t* ctx);

// returns 0 when end of buffer reached without command
// returns < 0 when there was an error, see defines
// returns cmd_id when successfully decoded the command
// buffer_start is modified upon return, when return value >0, the updated value
// should be provided again to continue scanning
// buffer_len provides length of remaining data in the buffer, excluding any
// NULL terminator.
// when return value > 0, argc and argv are updated
int cmd_interpreter_process(cmd_interpreter_ctx_t* ctx, char** buffer_start,
                            size_t buffer_len, int* argc, char** argv[]);

#define CMD_INTERPRETER_LINE_LENGTH_EXCEEDED -1
#define CMD_INTERPRETER_EMPTY_INPUT -2
#define CMD_INTERPRETER_INVALID_COMMAND -3

#endif /* _CMD_INTERPRETER_H_ */
