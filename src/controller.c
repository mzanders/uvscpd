#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#include "controller.h"
#include "tcpserver.h"



extern sig_atomic_t gsigterm_received;
extern sig_atomic_t gsighup_received;
extern sig_atomic_t gsigint_received;


void controller (const char * Config_File)
{
  openlog("uvscpd : ", LOG_PID, LOG_USER);

  tcpserver_start(NULL);

  while (1)
  {
    if (gsighup_received | gsigterm_received | gsigint_received)
    {
      tcpserver_stop();
      exit(0);
    }
    sleep(1);

  }


  closelog();

}
