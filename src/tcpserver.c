#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "syserror.h"
#include "tcpserver.h"
#include "tcpserver_worker.h"

#define NUM_CONNECTIONS 5

static const char *ModuleName = "TCPServer";
static int tcpserver_running = 0;

pthread_t dispatch_tid;

typedef struct {
  pthread_t thread_tid;
  sem_t start_sem;
  int connfd;
  const char * can_bus;
  pthread_mutex_t connfd_lock;
} Thread;

Thread *tptr;
/* thread ID */
/* # connections handled */
/* array of Thread structures; calloc'ed */
int listenfd, nthreads;
socklen_t addrlen;
pthread_mutex_t mlock;

void *dispatch_thread(void *arg);
void *worker_thread(void *arg);

void tcpserver_start(const char * can_bus, uint32_t ip_addr, uint16_t port) {
  int i;
  struct sockaddr_in servaddr;

  assert(tcpserver_running == 0);
  /* Create a socket */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    SysMError("socket");

  int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    SysMError("setsockopt(SO_REUSEADDR) failed");

  /* Bind to the created socket */
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = ip_addr;
  servaddr.sin_port = htons(port);

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    SysMError("bind");

  /* set the socket in passive listen mode */
  if ((listen(listenfd, 5)) < 0)
    SysMError("listen");

  /* create the thread pool */
  nthreads = NUM_CONNECTIONS;
  tptr = calloc(nthreads, sizeof(Thread));
  if (tptr == NULL)
    SysMError("thread calloc");

  /* create worker threads first, they will block on the semaphore */
  for (i = 0; i < nthreads; i++) {
    sem_init(&(tptr[i].start_sem), 0, 0);
    tptr[i].can_bus = can_bus;
    if (pthread_create(&tptr[i].thread_tid, NULL, &worker_thread, &(tptr[i])) !=
        0)
      NonSysError(ModuleName, "pthread_create worker");
  }

  /* create the dispatcher thread */
  if (pthread_create(&dispatch_tid, NULL, &dispatch_thread, NULL) != 0)
    NonSysError(ModuleName, "pthread_create dispatch");

  tcpserver_running = 1;
  return;
}

void *dispatch_thread(void *arg) {
  int connfd;
  struct sockaddr_in cliaddr;
  socklen_t clilen;
  int found, i;

  while (1) {
    clilen = sizeof(cliaddr);

    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
      SysMError("thread accept");

    found = 0;
    i = 0;
    while ((found == 0) && (i < nthreads)) {
      if (pthread_mutex_lock(&(tptr[i].connfd_lock)) != 0)
        NonSysError("TCPServer", "dispatch mutex lock");

      if (tptr[i].connfd == 0) {
        found = 1;
        tptr[i].connfd = connfd;
        sem_post(&(tptr[i].start_sem));
      }

      if (pthread_mutex_unlock(&(tptr[i].connfd_lock)) != 0)
        NonSysError("TCPServer", "dispatch mutex unlock");

      i++;
    }

    if (found == 0) {
      if (close(connfd) < 0)
        SysMError("thread close connection");
    }
  }
}

void *worker_thread(void *arg) {
  Thread *info = arg;
  int connection;
  int status;

  while (1) {

    if ((status = sem_wait(&(info->start_sem))) != 0) {
      if (status != EINTR)
        SysMError("worker semaphore wait");
    } else {
      if (pthread_mutex_lock(&(info->connfd_lock)) != 0)
        NonSysError("TCPServer", "worker mutex Lock");

      connection = info->connfd;

      if (pthread_mutex_unlock(&(info->connfd_lock)) != 0)
        NonSysError("TCPServer", "worker mutex unlock");

      if (connection != 0) {
        tcpserver_work(connection, info->can_bus);

        if (close(connection) < 0)
          SysMError("thread close FD");

        if (pthread_mutex_lock(&(info->connfd_lock)) != 0)
          NonSysError("TCPServer", "worker mutex Lock");

        info->connfd = 0;

        if (pthread_mutex_unlock(&(info->connfd_lock)) != 0)
          NonSysError("TCPServer", "worker mutex unlock");
      }
    }
  }
}

void tcpserver_stop(void) {
  int i;
  void *res;

  /* stop the dispatcher first */
  if (pthread_cancel(dispatch_tid) < 0)
    NonSysError(ModuleName, "pthread_cancel");
  if (pthread_join(dispatch_tid, &res) < 0)
    NonSysError(ModuleName, "pthread_cancel");
  free(res);
  if (close(listenfd) < 0)
    SysMError("Close listener");

  /* stop all the workers */
  for (i = 0; i < nthreads; i++) {
    if (pthread_cancel(tptr[i].thread_tid) < 0)
      NonSysError(ModuleName, "pthread_cancel");
  }

  for (i = 0; i < nthreads; i++) {
    if (pthread_join(tptr[i].thread_tid, &res) < 0)
      NonSysError(ModuleName, "pthread_join");
    free(res);

    if (tptr[i].connfd != 0) {
      if (close(tptr[i].connfd) < 0)
        SysMError("Close cleanup");
      tptr[i].connfd = 0;
    }

    if (sem_destroy(&(tptr[i].start_sem)) < 0)
      SysMError("close sem destroy");
  }

  free(tptr);

  return;
}
