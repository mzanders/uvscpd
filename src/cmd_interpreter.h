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

#ifndef _CMD_INTERPRETER_H_
#define _CMD_INTERPRETER_H_

#include <stdlib.h>

typedef struct cmd_interpreter_ctx cmd_interpreter_ctx_t;

// callback prototype return 0 when OK, return -1 upon error
typedef int (*cmd_interpreter_callback_t)(void *object, int argc, char * argv[]);

typedef struct {
  const char* cmd_string;  // ascii string for this command
  cmd_interpreter_callback_t callback;
} cmd_interpreter_cmd_list_t;

// create a context for the command interpreter to work in
cmd_interpreter_ctx_t* cmd_interpreter_ctx_create(
    const cmd_interpreter_cmd_list_t* cmd_list,  // command list structure
    int num_commands,                            // number of elements in above
    int max_argc,            // max number of arguments that can be used
    int to_lower,            // convert all input to lower case (make sure cmd_string is all lower too!)
    size_t max_line_length,  // used for allocating internal buffer
    const char* delimiters   // list of delimiters between command & arguments
);

// destroy the command interpreter and free all memory
void * cmd_interpreter_free(cmd_interpreter_ctx_t* ctx);

// repeat the last received command
int cmd_interpreter_repeat(cmd_interpreter_ctx_t *ctx, void *obj);

// returns 0 when end of buffer reached without command
// returns < 0 when there was an error, see defines
// returns cmd_id when successfully decoded the command
// buffer_start is modified upon return, when return value >0, the updated value
// should be provided again to continue scanning
// buffer_len provides length of remaining data in the buffer, excluding any
// NULL terminator.
// when return value > 0, a callback has been called which returned 0
// obj will be passed on to the called functions
int cmd_interpreter_process(cmd_interpreter_ctx_t* ctx, char** buffer_start,
                            size_t buffer_len, void * obj);

#define CMD_INTERPRETER_NO_MORE_DATA -1
#define CMD_INTERPRETER_LINE_LENGTH_EXCEEDED -2
#define CMD_INTERPRETER_EMPTY_INPUT -3
#define CMD_INTERPRETER_INVALID_COMMAND -4

#endif /* _CMD_INTERPRETER_H_ */
