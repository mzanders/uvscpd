// uvscpd - Minimalist VSCP Daemon
// Copyright (C) 2019 Maarten Zanders
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
