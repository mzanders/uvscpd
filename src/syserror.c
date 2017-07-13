#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include "syserror.h"

void SysError(const char * Module, const char * Fct)
{
   syslog(LOG_ERR, "%s - %s - %s", Module, Fct, strerror(errno));
   exit (errno);
}

void NonSysError(const char * Module, const char * Fct)
{
   syslog(LOG_ERR, "%s - %s", Module, Fct);
   exit (-1);
}
