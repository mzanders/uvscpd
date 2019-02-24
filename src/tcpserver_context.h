#ifndef _TCPSERVER_CONTEXT_H_
#define _TCPSERVER_CONTEXT_H_

#include "cmd_interpreter.h"
#include "vscp_buffer.h"

typedef enum { normal, loop } servermode_t;

typedef struct {
  int tcpfd;
  servermode_t mode;
  int can_socket;
  char command_buffer[120];
  int command_buffer_wp;
  int stop_thread;
  cmd_interpreter_ctx_t *cmd_interpreter;
  int user_ok;
  int password_ok;
  vscp_buffer_ctx_t * rx_buffer;
} context_t;

#endif /* _TCPSERVER_CONTEXT_H_ */
