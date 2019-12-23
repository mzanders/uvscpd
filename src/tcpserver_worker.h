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

#ifndef _TCPSERVER_WORKER_H_
#define _TCPSERVER_WORKER_H_

#include <unistd.h>
#include <time.h>
#include "tcpserver_context.h"

  /* the actual work being done in a tcpserver */
  void tcpserver_work(int connfd, const char * can_bus, time_t started);
  int status_reply(context_t * context, int error, char *msg);
  ssize_t writen(context_t * context, const void *vptr, size_t n);
#endif /* #ifndef _TCPSERVER_WORKER_H_ */
