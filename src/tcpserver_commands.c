#include "tcpserver_commands.h"
#include "tcpserver_context.h"
#include "tcpserver_worker.h"

int do_noop(void *obj, int argc, char *argv[]);
int do_quit(void *obj, int argc, char *argv[]);
int do_test(void *obj, int argc, char *argv[]);

const cmd_interpreter_cmd_list_t command_descr[] = {
    {"noop", do_noop}, {"quit", do_quit}, {"test", do_test}};

const int command_descr_num =
    sizeof(command_descr) / sizeof(cmd_interpreter_cmd_list_t);

int do_noop(void *obj, int argc, char *argv[]) {
  if (argc != 1) {
    return -1;
  }
  return 0;
}

int do_quit(void *obj, int argc, char *argv[]) {
  context_t *context = (context_t *)obj;
  if (argc != 1) {
    return -1;
  }

  context->stop_thread = 1;
  return 0;
}

int do_test(void *obj, int argc, char *argv[]) {
  if (argc != 2) {
    return -1;
  }
  return 0;
}
