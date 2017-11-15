#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "syserror.h"
#include "tcpserver_worker.h"

static const char * ModuleName = "TCPServerWorker";

typedef enum {normal, loop} servermode_t;

typedef struct
{
   int tcpfd;
   servermode_t mode;
   int can_socket;
} context_t;

/* helper functions */
ssize_t writen(int fd, const void *vptr, size_t n);
void handle_noop(char* command);
void handle_quit(char* command);
void tcpserver_work_cleanup(void * context);
int status_reply (int fd, int error, char* msg);
int print_vscp_frame(context_t context, struct can_frame frame);
int nbytes;

typedef struct
{
  char* command;
  char* long_command;
  void (*cmd_handler)(char* command);
} command_descr_t;

const command_descr_t command_descr[] =
{
   {"noop", "noop", handle_noop},
   {"quit", "quit", handle_quit},
   // insert commands above this line
   {"last", "last", 0}
};

void tcpserver_work(int connfd)
{
  ssize_t n;
  char buf[120];
  context_t context;
  char * welcome_message = "uvscpd V?\n\rCopyright (c) 2017, Maarten Zanders <maarten.zanders@gmail.com>\n\r";
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct can_frame frame;

  context.tcpfd = connfd;
  context.mode = normal;
  context.can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);

  strcpy(ifr.ifr_name, "can0" );
  ioctl(context.can_socket, SIOCGIFINDEX, &ifr);
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  bind(context.can_socket, (struct sockaddr *)&addr, sizeof(addr));

  pthread_cleanup_push(tcpserver_work_cleanup, &context);

  writen(context.tcpfd, welcome_message, strlen(welcome_message));
  status_reply(context.tcpfd, 0, "Success.");

  while(1)
  {
    nbytes = read(context.can_socket, &frame, sizeof(struct can_frame));

    if (nbytes < 0 && errno != EINTR)
    {
        return;
    }
    else
    {
      if(errno != EINTR)
         print_vscp_frame(context, frame);
    }
  }

/*

  again:
  while ( (n = read(context.tcpfd, buf, 120)) > 0)
  {
    writen(context.tcpfd, buf, n);
  }
  if (n < 0 && errno == EINTR)
    goto again;
  else if (n < 0)
    SysMError("thread read");
*/

  pthread_cleanup_pop(1);
}


void tcpserver_work_cleanup(void * context)
{
   return;
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

int status_reply (int fd, int error, char* msg)
{
   char buffer[120];
   buffer[0] = 0;

   if(error)
      strcpy(buffer, "-");
   else
      strcpy(buffer, "+");

   strcat(buffer, "OK");

   if(msg != 0)
   {
     strcat(buffer, " - ");
     strncat(buffer, msg, sizeof(buffer)-strlen(buffer)-3);
   }
   strcat (buffer, "\n\r");

   return writen(fd, buffer, strlen(buffer));
}

int print_vscp_frame(context_t context, struct can_frame frame)
{
   char buffer[120];
   char minibuf[10];

   sprintf(buffer, "0x%08X", frame.can_id);
   for(int i=0; i<frame.can_dlc; i++)
   {
     sprintf(minibuf, ",0x%02X",frame.data[i]);
     strcat(buffer, minibuf);
   }
   strcat(buffer, "\n\r");
   return writen(context.tcpfd,buffer,strlen(buffer));
}


void handle_noop(char* command)
{

}
void handle_quit(char* command)
{

}
