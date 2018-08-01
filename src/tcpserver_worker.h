#ifndef _TCPSERVER_WORKER_H_
#define _TCPSERVER_WORKER_H_

  /* the actual work being done in a tcpserver */
  void tcpserver_work(int connfd);
  int status_reply(int fd, int error, char *msg);

#endif /* #ifndef _TCPSERVER_WORKER_H_ */
