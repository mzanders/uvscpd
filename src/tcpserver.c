#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "tcpserver.h"
#include "syserror.h"

#define NUM_CONNECTIONS 5
#define TCPSERVER_PORT 33000

static const char * ModuleName = "TCPServer";
static int tcpserver_running = 0;

typedef struct {
  pthread_t thread_tid;
  long thread_count;
} Thread;

Thread *tptr;
/* thread ID */
/* # connections handled */
/* array of Thread structures; calloc'ed */
int listenfd, nthreads;
socklen_t addrlen;
pthread_mutex_t mlock;

void * tcpserver_thread(void *arg);
void tcpserver_work(int connfd);

void tcpserver_start (const char * Configuration)
{
  int i;
  struct sockaddr_in servaddr;

  assert(tcpserver_running == 0);

  /* Create a socket */
  if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     SysMError("socket");

  /* Bind to the created socket */
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(TCPSERVER_PORT);

  if(bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    SysMError ("bind");

  /* set the socket in passive listen mode */
  if((listen(listenfd, 5)) < 0)
    SysMError ("listen");

  /* create the thread pool */
  nthreads = NUM_CONNECTIONS;
  tptr = calloc(nthreads, sizeof(Thread));
  if (tptr == NULL)
    SysMError("thread calloc");

  for (i = 0; i < nthreads; i++)
  {
     if(pthread_create(&tptr[i].thread_tid, NULL, &tcpserver_thread, NULL) != 0)
       NonSysError(ModuleName, "pthread_create");
  }

  tcpserver_running = 1;
  return;
}

void * tcpserver_thread(void *arg)
{
  int connfd;
  struct sockaddr_in cliaddr;
  socklen_t clilen;

   while(1)
   {
     clilen = sizeof(cliaddr);
     if(pthread_mutex_lock(&mlock) != 0)
       NonSysError(ModuleName, "thread mutex lock");

     if((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) < 0 )
       SysMError("thread accept");

     if(pthread_mutex_unlock(&mlock) != 0)
       NonSysError(ModuleName, "thread mutex unlock");

     tcpserver_work(connfd);

     if(close(connfd) < 0)
       SysMError("thread close FD");

   }
}

/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n)
{
  size_t nleft;
  ssize_t nwritten;
  const char *ptr;
  ptr = vptr;
  nleft = n;
  while (nleft > 0)
  {
    if ( (nwritten = write(fd, ptr, nleft)) <= 0)
    {
      if (nwritten < 0 && errno == EINTR)
        nwritten = 0;   /* and call write() again */
      else
        return (-1);    /* error */
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n);
}

void tcpserver_work(int connfd)
{
  ssize_t n;
  char buf[120];

  char * WelcomeMsg = "Welcome to this test server...\n";

  writen(connfd, WelcomeMsg, strlen(WelcomeMsg));

  again:
  while ( (n = read(connfd, buf, 120)) > 0)
  {
    writen(connfd, buf, n);
  }
  if (n < 0 && errno == EINTR)
    goto again;
  else if (n < 0)
    SysMError("thread read");
}


void tcpserver_stop (void)
{

   free(tptr);
   close(listenfd);
   return;

}
