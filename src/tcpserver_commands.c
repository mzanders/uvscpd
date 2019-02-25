#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"
#include "unistd.h"
#include "version.h"
#include "vscp.h"
#include "vscp_buffer.h"

/* Global stuff */
char *cmd_user = NULL;
char *cmd_password = NULL;

static int do_noop(void *obj, int argc, char *argv[]);
static int do_quit(void *obj, int argc, char *argv[]);
static int do_test(void *obj, int argc, char *argv[]);
static int do_repeat(void *obj, int argc, char *argv[]);
static int do_user(void *obj, int argc, char *argv[]);
static int do_password(void *obj, int argc, char *argv[]);
static int do_restart(void *obj, int argc, char *argv[]);
static int do_send(void *obj, int argc, char *argv[]);
static int do_retrieve(void *obj, int argc, char *argv[]);
static int do_rcvloop(void *obj, int argc, char *argv[]);
static int do_quitloop(void *obj, int argc, char *argv[]);
static int do_checkdata(void *obj, int argc, char *argv[]);
static int do_clearall(void *obj, int argc, char *argv[]);
static int do_getguid(void *obj, int argc, char *argv[]);
static int do_setguid(void *obj, int argc, char *argv[]);
static int do_wcyd(void *obj, int argc, char *argv[]);
static int do_version(void *obj, int argc, char *argv[]);
static int do_stat(void *obj, int argc, char *argv[]);

const cmd_interpreter_cmd_list_t command_descr[] = {
    {"+", do_repeat},          {"noop", do_noop},
    {"quit", do_quit},         {"test", do_test},
    {"user", do_user},         {"pass", do_password},
    {"restart", do_restart},   {"shutdown", do_restart},
    {"send", do_send},         {"retr", do_retrieve},
    {"rcvloop", do_rcvloop},   {"quitloop", do_quitloop},
    {"cdata", do_checkdata},   {"checkdata", do_checkdata},
    {"clra", do_clearall},     {"ggid", do_getguid},
    {"getguid", do_getguid},   {"sgid", do_setguid},
    {"setguid", do_setguid},   {"wcyd", do_wcyd},
    {"whatcanyoudo", do_wcyd}, {"vers", do_version},
    {"version", do_version},   {"stat", do_stat}};

const int command_descr_num =
    sizeof(command_descr) / sizeof(cmd_interpreter_cmd_list_t);

static int access_ok(context_t *context) {
  return context->password_ok && context->user_ok;
}

int do_noop(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

int do_quit(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  status_reply(context->tcpfd, 0, "bye");
  context->stop_thread = 1;
  return 0;
}

int do_test(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  if (access_ok(context))
    status_reply(context->tcpfd, 0, NULL);
  else
    status_reply(context->tcpfd, 1, "no access");

  return 0;
}

static int do_repeat(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  cmd_interpreter_disable_history(context->cmd_interpreter);
  if (argc == 1) {
    return cmd_interpreter_repeat(context->cmd_interpreter, obj);
  }

  return CMD_WRONG_ARGUMENT_COUNT;
}

static int do_user(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 2) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  if (cmd_user == NULL) {
    context->user_ok = 1;
    status_reply(context->tcpfd, 0, "no user configured");
    return 0;
  }
  if (strcmp(argv[1], cmd_user) == 0) {
    context->user_ok = 1;
    status_reply(context->tcpfd, 0, NULL);
  } else {
    context->user_ok = 0;
    status_reply(context->tcpfd, 1, "invalid user");
  }
  return 0;
}

static int do_password(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 2) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  if (cmd_password == NULL) {
    context->password_ok = 1;
    status_reply(context->tcpfd, 0, "no password configured");
    return 0;
  }
  if (strcmp(argv[1], cmd_password) == 0) {
    context->password_ok = 1;
    status_reply(context->tcpfd, 0, NULL);
  } else {
    context->password_ok = 0;
    status_reply(context->tcpfd, 1, "invalid password");
  }
  return 0;
}

static int do_restart(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  status_reply(context->tcpfd, 1, "uvscpd is not capable of restart/shutdown");
  return 0;
}

// send head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3....
static int do_send(void *obj, int argc, char *argv[]) {
  vscp_msg_t msg;
  struct can_frame tx;
  context_t *context = (context_t *)obj;
  if (argc != 2) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  if (vscp_parse_msg(argv[1], &msg)) {
    status_reply(context->tcpfd, 1, "format error in CAN frame");
    return 0;
  }
  vscp_to_can(&msg, &tx);
  context->stat_tx_data += 4 + tx.can_dlc;
  context->stat_tx_frame++;
  if (write(context->can_socket, &tx, sizeof(struct can_frame)) !=
      sizeof(struct can_frame)) {
    status_reply(context->tcpfd, 1, "problem when writing to CAN socket");
    return 0;
  }
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_retrieve(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  unsigned int num_msgs;
  char guard;
  int empty_buffer = 0;
  vscp_msg_t msg;
  char buf[120];
  int n;
  if (argc > 2) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }

  if (argc == 2) {
    if (sscanf(argv[1], "%u%c", &num_msgs, &guard) != 1) {
      return CMD_FORMAT_ERROR;
    }
  } else
    num_msgs = 1;

  while (num_msgs > 0 && !empty_buffer) {
    empty_buffer = vscp_buffer_pop(context->rx_buffer, &msg);
    if (!empty_buffer) {
      n = print_vscp(&msg, buf, sizeof(buf));
      writen(context->tcpfd, buf, n);
    }
    num_msgs--;
  }

  if (empty_buffer)
    status_reply(context->tcpfd, 1, "No event(s) available");
  else
    status_reply(context->tcpfd, 0, NULL);

  return 0;
}

static int do_rcvloop(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  int empty_buffer = 0;
  char buf[120];
  int n;
  vscp_msg_t msg;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  context->mode = loop;

  clock_gettime(CLOCK_MONOTONIC_RAW, &(context->last_keepalive));
  status_reply(context->tcpfd, 0, NULL);

  while (!empty_buffer) {
    empty_buffer = vscp_buffer_pop(context->rx_buffer, &msg);
    if (!empty_buffer) {
      n = print_vscp(&msg, buf, sizeof(buf));
      writen(context->tcpfd, buf, n);
    }
  }
  return 0;
}

static int do_quitloop(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  context->mode = normal;
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_checkdata(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  char buf[20];
  int n;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  n = snprintf(buf, 20, "%u \r\n", vscp_buffer_used(context->rx_buffer));
  writen(context->tcpfd, buf, n);
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_clearall(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  vscp_buffer_flush(context->rx_buffer);
  status_reply(context->tcpfd, 0, "All events cleared.");
  return 0;
}

static int do_getguid(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  char buf[56];
  int n;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  n = vscp_print_guid(buf, sizeof(buf), &(context->guid));
  if (n >= sizeof(buf) - 2) {
    status_reply(context->tcpfd, 1, "Buffer overflow");
    return 0;
  }
  buf[n] = '\r';
  buf[n + 1] = '\n';
  writen(context->tcpfd, buf, n + 2);
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_setguid(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  vscp_guid_t guid;

  if (argc != 2) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }

  if (vscp_strtoguid(argv[1], &guid)) {
    status_reply(context->tcpfd, 1, "Invalid GUID");
  } else {
    context->guid = guid;
    status_reply(context->tcpfd, 0, NULL);
  }
  return 0;
}

static int do_wcyd(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  const char string[] = "00-00-00-00-00-00-80-28\r\n";
  writen(context->tcpfd, string, strlen(string));
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_version(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  char string[20];
  snprintf(string, sizeof(string), "%s,%s,%s,%s\r\n", VERSION_MAJOR,
           VERSION_MINOR, VERSION_SUBMINOR, VERSION_BUILD);
  writen(context->tcpfd, string, strlen(string));
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}

static int do_stat(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return CMD_WRONG_ARGUMENT_COUNT;
  }
  char string[100];
  snprintf(string, sizeof(string), "0,0,0,%u,%u,%u,%u\r\n",
           context->stat_rx_data, context->stat_rx_frame,
           context->stat_tx_data, context->stat_tx_frame);
  writen(context->tcpfd, string, strlen(string));
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}
