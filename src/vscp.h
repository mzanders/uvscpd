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

#ifndef _VSCP_H_
#define _VSCP_H_

#include <linux/can/raw.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef struct vscp_guid { uint8_t guid[16]; } vscp_guid_t;

typedef struct {
  uint8_t head;
  uint8_t type;
  uint16_t class;
  vscp_guid_t guid;
  uint32_t hw_timestamp;
  time_t timestamp;
  uint8_t data_length;
  uint8_t data[8];
} vscp_msg_t;

int vscp_strtoguid(const char * input, vscp_guid_t * guid);

// parses "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
int vscp_parse_msg(const char *input, vscp_msg_t *msg, vscp_guid_t *my_guid);
void vscp_to_can(const vscp_msg_t *msg, struct can_frame *frame);

int can_to_vscp(const struct can_frame *frame,
                const struct timeval *hw_timestamp, vscp_msg_t *msg,
                vscp_guid_t *guid);
int print_vscp(const vscp_msg_t *msg, char *buffer, size_t buffer_size);
int vscp_print_guid(char *buffer, size_t buffer_size, const vscp_guid_t *guid);
#endif /* #ifndef _VSCP_H_ */
