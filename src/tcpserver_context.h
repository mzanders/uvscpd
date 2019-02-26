#ifndef _TCPSERVER_CONTEXT_H_
#define _TCPSERVER_CONTEXT_H_

#include <time.h>
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
  vscp_guid_t guid;
  vscp_buffer_ctx_t * rx_buffer;
  struct timespec last_keepalive;
  unsigned int stat_rx_data;
  unsigned int stat_rx_frame;
  unsigned int stat_tx_data;
  unsigned int stat_tx_frame;
  const char * can_bus;
  time_t started;
} context_t;

#endif /* _TCPSERVER_CONTEXT_H_ */
