#ifndef _TCPSERVER_COMMANDS_H_
#define _TCPSERVER_COMMANDS_H_
#include "cmd_interpreter.h"

extern const cmd_interpreter_cmd_list_t command_descr[];
extern const int command_descr_num;

extern char * cmd_user;
extern char * cmd_password;

#define CMD_WRONG_ARGUMENT_COUNT -10
#define CMD_FORMAT_ERROR         -11

#endif /* _TCPSERVER_COMMANDS_H_ */
