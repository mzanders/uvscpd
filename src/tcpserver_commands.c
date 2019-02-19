#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"
#include "unistd.h"
#include "vscp.h"

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

const cmd_interpreter_cmd_list_t command_descr[] = {
    {"+", do_repeat},        {"noop", do_noop},        {"quit", do_quit},
    {"test", do_test},       {"user", do_user},        {"pass", do_password},
    {"restart", do_restart}, {"shutdown", do_restart}, {"send", do_send}};

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
  if (write(context->can_socket, &tx, sizeof(struct can_frame)) !=
      sizeof(struct can_frame)) {
    status_reply(context->tcpfd, 1, "problem when writing to CAN socket");
    return 0;
  }
  status_reply(context->tcpfd, 0, NULL);
  return 0;
}
