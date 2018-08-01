#include <errno.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "cmd_interpreter.h"
#include "syserror.h"
#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"

static const char *ModuleName = "TCPServerWorker";

/* helper functions */
ssize_t writen(int fd, const void *vptr, size_t n);
void handle_noop(context_t *context);
void handle_quit(context_t *context);
void tcpserver_work_cleanup(void *context);
int status_reply(int fd, int error, char *msg);
int print_vscp_frame(context_t *context, struct can_frame frame);
int nbytes;

typedef struct {
  char *command;
  char *long_command;
  void (*cmd_handler)(context_t *context);
} command_descr_t;

void tcpserver_handle_input(context_t *context, char *buffer, ssize_t length);

void tcpserver_work(int connfd) {
  ssize_t n;
  char buf[120];
  context_t context;
  char *welcome_message = "uvscpd V?\n\rCopyright (c) 2018, Maarten Zanders "
                          "<maarten.zanders@gmail.com>\n\r";
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct can_frame;
  struct pollfd poll_fd[2];
  const int max_argc = 10;
  const int max_line_length = 128;

  context.tcpfd = connfd;
  context.mode = normal;
  context.can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  context.command_buffer_wp = 0;
  context.cmd_interpreter = cmd_interpreter_ctx_create(
      command_descr, command_descr_num, max_argc, 0, max_line_length, " ");
  pthread_cleanup_push(tcpserver_work_cleanup, context.cmd_interpreter);

  strcpy(ifr.ifr_name, "can0");
  ioctl(context.can_socket, SIOCGIFINDEX, &ifr);
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  bind(context.can_socket, (struct sockaddr *)&addr, sizeof(addr));

  writen(context.tcpfd, welcome_message, strlen(welcome_message));
  status_reply(context.tcpfd, 0, "Success.");

  // Set up the poll structure
  poll_fd[0].fd = context.tcpfd;
  poll_fd[0].events = POLLIN;
  poll_fd[0].revents = 0;
  poll_fd[1].fd = context.can_socket;
  poll_fd[1].events = POLLIN;
  poll_fd[1].revents = 0;

  context.stop_thread = 0;
  while (!context.stop_thread) {
    switch (context.mode) {
    case normal:
      n = read(context.tcpfd, buf, sizeof(buf));

      if (n < 0 && errno != EINTR) {
        context.stop_thread = 1;
      } else {
        if (errno != EINTR)
          tcpserver_handle_input(&context, buf, n);
      }

      break;

    case loop:
      // similar as above but add poll and CAN reception
      break;
    }
  }

  /*

    again:
    while ( (n = read(context.tcpfd, buf, 120)) > 0)
    {
      writen(context.tcpfd, buf, n);
    }
    if (n < 0 && errno == EINTR)
      goto again;
    else if (n < 0)
      SysMError("thread read");
  */

  pthread_cleanup_pop(1);
}

void tcpserver_work_cleanup(void *context) { cmd_interpreter_free(context); }

/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR)
        nwritten = 0; /* and call write() again */
      else
        return (-1); /* error */
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n);
}

int status_reply(int fd, int error, char *msg) {
  char buffer[120];
  buffer[0] = 0;

  if (error)
    strcpy(buffer, "-");
  else
    strcpy(buffer, "+");

  strcat(buffer, "OK");

  if (msg != 0) {
    strcat(buffer, " - ");
    strncat(buffer, msg, sizeof(buffer) - strlen(buffer) - 3);
  }
  strcat(buffer, "\n\r");

  return writen(fd, buffer, strlen(buffer));
}

int print_vscp_frame(context_t *context, struct can_frame frame) {
  char buffer[120];
  char minibuf[10];

  sprintf(buffer, "0x%08X", frame.can_id);
  for (int i = 0; i < frame.can_dlc; i++) {
    sprintf(minibuf, ",0x%02X", frame.data[i]);
    strcat(buffer, minibuf);
  }
  strcat(buffer, "\n\r");
  return writen(context->tcpfd, buffer, strlen(buffer));
}

void tcpserver_handle_input(context_t *context, char *buffer, ssize_t length) {
  char *saveptr = buffer;
  int rval = 1;

  do {
    rval = cmd_interpreter_process(context->cmd_interpreter, &saveptr,
                                   (length - (saveptr - buffer)), context);
    if (rval > 0)
      status_reply(context->tcpfd, 0, NULL);
    if (rval < 0) {
      switch (rval) {
      case CMD_INTERPRETER_LINE_LENGTH_EXCEEDED:
        status_reply(context->tcpfd, 1, "line length exceeded");
        break;

      case CMD_INTERPRETER_EMPTY_INPUT:
        status_reply(context->tcpfd, 1, "no input");
        break;
      case CMD_INTERPRETER_INVALID_COMMAND:
        status_reply(context->tcpfd, 1, "invalid command");
        break;
      case CMD_INTERPRETER_CALLBACK_RETURNED_ERROR:
        status_reply(context->tcpfd, 1, "error in command");
        break;
      }
    }

  } while (rval != 0);
}

void handle_noop(context_t *context) {}

void handle_quit(context_t *context) {
  status_reply(context->tcpfd, 0, NULL);
  context->stop_thread = 1;
}
