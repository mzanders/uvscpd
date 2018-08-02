#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"

static int do_noop(void *obj, int argc, char *argv[]);
static int do_quit(void *obj, int argc, char *argv[]);
static int do_test(void *obj, int argc, char *argv[]);
static int do_repeat(void *obj, int argc, char *argv[]);

const cmd_interpreter_cmd_list_t command_descr[] = {
    {"+", do_repeat},
    {"noop", do_noop},
    {"quit", do_quit},
    {"test", do_test}
};

const int command_descr_num =
    sizeof(command_descr) / sizeof(cmd_interpreter_cmd_list_t);

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
  status_reply(context->tcpfd, 0, "bye");
  return 0;
}

static int do_repeat(void *obj, int argc, char *argv[])
{
   context_t *context = (context_t *)obj;
      cmd_interpreter_disable_history(context->cmd_interpreter);
   if (argc == 1) {
      return cmd_interpreter_repeat(context->cmd_interpreter, obj);
   }

   return CMD_WRONG_ARGUMENT_COUNT;

}
