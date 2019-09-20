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

#ifndef _TCPSERVER_COMMANDS_H_
#define _TCPSERVER_COMMANDS_H_
#include "cmd_interpreter.h"

extern const cmd_interpreter_cmd_list_t command_descr[];
extern const int command_descr_num;

extern char * cmd_user;
extern char * cmd_password;

#define CMD_WRONG_ARGUMENT_COUNT -10
#define CMD_FORMAT_ERROR         -11

#endif /* _TCPSERVER_COMMANDS_H_ */
