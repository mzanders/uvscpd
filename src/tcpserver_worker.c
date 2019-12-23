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

#include <errno.h>
#include <fcntl.h>
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
#include <time.h>

#include "cmd_interpreter.h"
#include "syserror.h"
#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"
#include "config.h"
#include "vscp.h"
#include "vscp_buffer.h"

/* helper functions */
ssize_t writen(int fd, const void *vptr, size_t n);
void handle_noop(context_t *context);
void handle_quit(context_t *context);
void tcpserver_work_cleanup(void *context);
int status_reply(int fd, int error, char *msg);
int nbytes;

extern vscp_guid_t gGuid;

typedef struct {
  char *command;
  char *long_command;
  void (*cmd_handler)(context_t *context);
} command_descr_t;

void tcpserver_handle_input(context_t *context, char *buffer, ssize_t length);

int print_vscp_frame(context_t *context, const struct can_frame *frame) {
  char buffer[120];
  char minibuf[10];

  sprintf(buffer, "0x%08X", frame->can_id);
  for (int i = 0; i < frame->can_dlc; i++) {
    sprintf(minibuf, ",0x%02X", frame->data[i]);
    strcat(buffer, minibuf);
  }
  strcat(buffer, "\n\r");
  return writen(context->tcpfd, buffer, strlen(buffer));
}

void tcpserver_work(int connfd, const char * can_bus, time_t started){
  ssize_t n;
  char buf[120];
  context_t context;
  char *welcome_message =
      PACKAGE_STRING "\n\r"
      PACKAGE_BUGREPORT "\n\r";
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct can_frame frame;
  int sock_flags;
  struct pollfd poll_fd[2];
  const int max_argc = 10;
  const int max_line_length = 320;

  memset(&addr, 0, sizeof(struct sockaddr_can));

  context.user_ok = (cmd_user == NULL);
  context.password_ok = (cmd_password == NULL);
  context.stop_thread = 0;
  context.tcpfd = connfd;
  context.mode = normal;
  context.can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  context.command_buffer_wp = 0;
  context.guid = gGuid; /* initialize to default guid provided externally */
  context.cmd_interpreter = cmd_interpreter_ctx_create(
      command_descr, command_descr_num, max_argc, 1, max_line_length, " ");
  context.rx_buffer = vscp_buffer_ctx_create(100);
  context.stat_rx_data = 0;
  context.stat_rx_frame = 0;
  context.stat_tx_data = 0;
  context.stat_tx_frame = 0;
  context.can_bus = can_bus;
  context.started = started;
  context.filter.can_id = 0x0;
  context.filter.can_mask = 0x0;
  pthread_cleanup_push(tcpserver_work_cleanup, &context);

  writen(context.tcpfd, welcome_message, strlen(welcome_message));

  strncpy(ifr.ifr_name, can_bus, IFNAMSIZ - 1);
  if (ioctl(context.can_socket, SIOCGIFINDEX, &ifr) == -1) {
    snprintf(buf, 120, "interface [%s] error: %s", can_bus, strerror(errno));
    status_reply(context.tcpfd, 1, buf);
    context.stop_thread = 1;
  } else {
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    /* set to non-blocking mode */
    sock_flags = fcntl(context.can_socket, F_GETFL, 0);
    fcntl(context.can_socket, F_SETFL, sock_flags | O_NONBLOCK);

    if (bind(context.can_socket, (struct sockaddr *)&addr, sizeof(addr)) ==
        -1) {
      snprintf(buf, 120, "error binding to CAN bus: %s", strerror(errno));
      status_reply(context.tcpfd, 1, buf);
      context.stop_thread = 1;
    } else {
      snprintf(buf, 120, "Success, connected to %s", can_bus);
      status_reply(context.tcpfd, 0, buf);
    }
  }

  while (!context.stop_thread) {
    // Set up the poll structure
    poll_fd[0].fd = context.tcpfd;
    poll_fd[0].events = POLLIN;
    poll_fd[0].revents = 0;
    poll_fd[1].fd = context.can_socket;
    poll_fd[1].events = POLLIN;
    poll_fd[1].revents = 0;

    int poll_rv;
    poll_rv = poll(poll_fd, 2, 200);

    if (poll_rv < 0) {
      snprintf(buf, 120, "Poll error - %s", strerror(errno));
      status_reply(context.tcpfd, 1, buf);
      context.stop_thread = 1;
    } else if (poll_rv > 0) {

      /* handle TCP events */
      if (poll_fd[0].revents & POLLIN) {
        n = read(context.tcpfd, buf, sizeof(buf));
        if(n<=0){
          context.stop_thread = 1; /* error or closed socket */
        } else {
          tcpserver_handle_input(&context, buf, n);
        }
      }
      if (poll_fd[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        context.stop_thread = 1;
      }

      /* handle CAN events */
      if (poll_fd[1].revents & POLLIN) {
        if (read(context.can_socket, &frame, sizeof(struct can_frame)) ==
            sizeof(struct can_frame)) {
          vscp_msg_t msg;
          struct timeval tv;
          ioctl(context.can_socket, SIOCGSTAMP, &tv);
          if (!can_to_vscp(&frame, &tv, &msg, &(context.guid))) {
            context.stat_rx_data += frame.can_dlc + 4;
            context.stat_rx_frame++;
            if (context.mode == loop) {
              n = print_vscp(&msg, buf, sizeof(buf));
              writen(context.tcpfd, buf, n);
            } else {
              vscp_buffer_push(context.rx_buffer, &msg);
            }
          }
        }
      }
      if (poll_fd[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        status_reply(context.tcpfd, 1, "CAN Disconnected - bye!");
        context.stop_thread = 1;
      }
    }
    /* POLL TIMEOUT */
    else if (poll_rv == 0) {
      if (context.mode == loop) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        /* this is not really accurate, especially on the first run, but close
         * enough and it will do for the purpose */
        if ((now.tv_sec - context.last_keepalive.tv_sec) > 1) {
          status_reply(context.tcpfd, 0, NULL);
          context.last_keepalive = now;
        }
      }
    }
  }
  pthread_cleanup_pop(1);
}

void tcpserver_work_cleanup(void *context) {
  context_t *ctx = (context_t *)context;
  cmd_interpreter_free(ctx->cmd_interpreter);
  vscp_buffer_free(ctx->rx_buffer);
}

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

  if (msg != 0 && strlen(msg) > 0) {
    strcat(buffer, " - ");
    strncat(buffer, msg, sizeof(buffer) - strlen(buffer) - 3);
  }
  strcat(buffer, "\n\r");

  return writen(fd, buffer, strlen(buffer));
}

void tcpserver_handle_input(context_t *context, char *buffer, ssize_t length) {
  char *saveptr = buffer;
  int rval = 1;

  do {
    rval = cmd_interpreter_process(context->cmd_interpreter, &saveptr,
                                   (length - (saveptr - buffer)), context);
    if (rval < 0) {
      switch (rval) {
      case CMD_INTERPRETER_NO_MORE_DATA:
        break;
      case CMD_INTERPRETER_LINE_LENGTH_EXCEEDED:
        status_reply(context->tcpfd, 1, "line length exceeded");
        break;
      case CMD_INTERPRETER_EMPTY_INPUT:
        status_reply(context->tcpfd, 1, "no input");
        break;
      case CMD_INTERPRETER_INVALID_COMMAND:
        status_reply(context->tcpfd, 1, "invalid command");
        break;
      case CMD_WRONG_ARGUMENT_COUNT:
        status_reply(context->tcpfd, 1, "wrong number of arguments");
        break;
      default:
        status_reply(context->tcpfd, 1, NULL);
        break;
      }
    }
  } while (rval != CMD_INTERPRETER_NO_MORE_DATA);
}
