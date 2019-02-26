#ifndef _TCPSERVER_WORKER_H_
#define _TCPSERVER_WORKER_H_

#include <unistd.h>
#include <time.h>

  /* the actual work being done in a tcpserver */
  void tcpserver_work(int connfd, const char * can_bus, time_t started);
  int status_reply(int fd, int error, char *msg);
  ssize_t writen(int fd, const void *vptr, size_t n);
#endif /* #ifndef _TCPSERVER_WORKER_H_ */
