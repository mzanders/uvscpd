#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "controller.h"
#include "tcpserver_commands.h"
#include "tcpserver_worker.h"

#define TCPSERVER_PORT 8598

void uvscpd_show_version(void);
void uvscpd_show_help(void);

sig_atomic_t gsigterm_received = 0;
sig_atomic_t gsighup_received = 0;
sig_atomic_t gsigint_received = 0;
int gDaemonize = 1;

void signal_handler(int signal_number) {
  switch (signal_number) {
  case SIGHUP:
    gsighup_received = 1;

  case SIGTERM:
    gsigterm_received = 1;
    break;

  case SIGINT:
    gsigint_received = 1;
    break;

  default:
    break;
  }
}

int main(int argc, char *argv[]) {
  int next_option = 0;
  int longindex;
  pid_t pid, sid;
  char *endptr;

  uint32_t ip_addr = 0; /* any address...*/
  uint16_t port = TCPSERVER_PORT;
  char *can_bus;

  const char *const short_options = "hvsUP";
  const struct option long_options[] = {
      // name, has_arg, flag, val
      {"help", 0, NULL, 'h'},      {"version", 0, NULL, 'v'},
      {"stay", 0, &gDaemonize, 0}, {"user", 1, NULL, 'U'},
      {"password", 1, NULL, 'P'},  {"canbus", 1, NULL, 'c'},
      {"ip", 1, NULL, 'i'},        {"port", 1, NULL, 'p'},
      {NULL, 0, NULL, 0}};
  struct sigaction sa;

  while ((next_option = getopt_long(argc, argv, short_options, long_options,
                                    &longindex)) != -1) {
    switch (next_option) {
    case 0:
      break;

    case 'h':
      uvscpd_show_help();
      exit(0);
      break;

    case 'v':
      uvscpd_show_version();
      exit(0);
      break;

    case 'U':
      cmd_user = strdup(optarg);
      break;

    case 'P':
      cmd_password = strdup(optarg);
      break;

    case 'c':
      can_bus = strdup(optarg);
      break;

    case 'p':
      endptr = optarg;
      port = strtol(optarg, &endptr, 10);
      if (endptr == NULL) {
        fprintf(stderr, "invalid port\n");
        exit(-1);
      }
      break;

    case 'i':
      if (inet_pton(AF_INET, optarg, &ip_addr) != 1) {
        fprintf(stderr, "invalid ip address\n");
        exit(-1);
      }
      break;

    case '?':
    default:
      uvscpd_show_help();
      exit(-1);
    }
  }

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &signal_handler;
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  if (gDaemonize) {
    // Fork child
    if (0 > (pid = fork())) {
      fprintf(stderr, "Failed to fork.\n");
      return -1;
    } else if (0 != pid) {
      exit(0); // Parent exits
    }
    sid = setsid(); // Become session leader
    if (sid < 0) {
      // Failure
      fprintf(stderr, "uvscpd: failed to become session leader.\n");
      return -1;
    }

    umask(0); // Clear file mode creation mask

    // Close the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    syslog(LOG_INFO, "uvscpd started");

    if (open("/", 0)) {
      syslog(LOG_CRIT, "uvscpd: open / not 0: %m");
    }

    dup2(0, 1);
    dup2(0, 2);
  }
  controller(can_bus, ip_addr, port);
}

///////////////////////////////////////////////////////////////////////////////
// copyleft
void uvscpd_show_version(void) {
  printf("uvscpd 0.1\n");
  printf("Copyright (C) 2017 Maarten Zanders\n");
  printf("License GPLv3+: GNU GPL version 3 or later ");
  printf("<http://gnu.org/licenses/gpl.html>\n");
  printf(
      "This is free software: you are free to change and redistribute it.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

///////////////////////////////////////////////////////////////////////////////
// help
void uvscpd_show_help(void) {
  printf("No usage information so far... sorry!\n");
}
