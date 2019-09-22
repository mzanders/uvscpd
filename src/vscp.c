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

#include <stdio.h>
#include <string.h>

#include "vscp.h"

//          0    1      2   3     4         5       6     7     8     9
// parses "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
// datetime YYYY-MM-DDTHH:MM:DD
// send
// 0,30,11,0,0000-00-00t16:00:00z,0,00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00,0xff,0x00,0xaa,0x55
// send
// 96,512,9,0,1272-23-95T05:127:00Z,0,00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x03,0xD0

int vscp_parse_msg(const char *input, vscp_msg_t *msg, vscp_guid_t *my_guid) {
  const char *ptr = input;
  char *temp;
  unsigned int value;
  int field = 0;
  int level2 = 0;

  msg->data_length = 0;

  /* we're not parsing these values - lazy */
  msg->hw_timestamp = 0;
  msg->timestamp = 0;

  while (*ptr != 0) {

    const char *seek = ptr;
    char guard;

    while (*seek != ',' && *seek != 0)
      seek++;

    temp = strndup(ptr, seek - ptr);

    switch (field) {
    case 0 ... 2:
    case 7 ... 30:
      if (temp[0] == '0' && temp[1] == 'x') {
        if (sscanf(temp, "%X%c", &value, &guard) != 1) {
          return -1;
        }
      } else {
        if (sscanf(temp, "%u%c", &value, &guard) != 1) {
          return -1;
        }
      }
      break;
    case 31:
      return -1;
      break;
    }

    switch (field) {
    case 0:
      if (value <= UINT8_MAX)
        msg->head = (uint8_t)value;
      else
        return -1;
      break;

    case 1:
      if (value < 512)
        msg->class = (uint16_t)value;
      else if (value < 1024) {
        msg->class = (uint16_t)value - 512;
        level2 = 1;
      } else
        return -1;
      break;

    case 2:
      if (value <= UINT8_MAX)
        msg->type = (uint8_t)value;
      else
        return -1;
      break;

    case 7 ... 30:
      if (level2) {
        if (field < 23 && my_guid->guid[field - 7] != value) {
          return -1;
        } else if (field >= 23) {
          if (value <= UINT8_MAX)
            msg->data[field - 23] = (uint8_t)value;
          else {
            return -1;
          }
          msg->data_length++;
        }
      } else {
        if (field == 15) {
          return -1;
        }
        if (value <= UINT8_MAX)
          msg->data[field - 7] = (uint8_t)value;
        else
          return -1;
        msg->data_length++;
      }
      break;
    }

    ptr = seek;
    if (*ptr != 0)
      ptr++;
    field++;
    free(temp);
  }
  if (field > 6)
    return 0;
  else
    return -1;
}

void vscp_to_can(const vscp_msg_t *msg, struct can_frame *frame) {
  int i;
  frame->can_id = CAN_EFF_FLAG | ((msg->head & 0xF0) << 21) | msg->class << 16 |
                  msg->type << 8;
  frame->can_dlc = msg->data_length;
  for (i = 0; i < msg->data_length; i++)
    frame->data[i] = msg->data[i];
}

int can_to_vscp(const struct can_frame *frame,
                const struct timeval *hw_timestamp, vscp_msg_t *msg,
                vscp_guid_t *guid) {
  int i;
  if ((frame->can_id & CAN_EFF_FLAG) == 0) {
    return -1;
  }
  if (frame->can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG)) {
    return -1;
  }
  if (frame->can_dlc >= 8) {
    return -1;
  }
  msg->head = (uint8_t)((frame->can_id & 0x1E000000U) >> 21);
  msg->class = (uint16_t)((frame->can_id & 0x01FF0000U) >> 16);
  msg->type = (uint8_t)((frame->can_id & 0x0000FF00U) >> 8);
  msg->guid = *guid;
  msg->guid.guid[15] = (uint8_t)((frame->can_id & 0x000000FFU));
  msg->data_length = frame->can_dlc;
  msg->timestamp = time(NULL);
  msg->hw_timestamp = (uint32_t)(hw_timestamp->tv_usec)
                         + (uint32_t)(hw_timestamp->tv_sec * 1000000);

  for (i = 0; i < msg->data_length; i++) {
    msg->data[i] = frame->data[i];
  }
  return 0;
}
// "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
// datetime YYYY-MM-DDTHH:MM:DD
int print_vscp(const vscp_msg_t *msg, char *buffer, size_t buffer_size) {
  int i;
  char timebuffer[32];
  char guidbuffer[56];

  struct tm *tmp;
  tmp = gmtime(&(msg->timestamp));

  if (tmp != NULL)
    strftime(timebuffer, 32, "%FT%H:%M:%S", tmp);
  else
    snprintf(timebuffer, 32, "");

  vscp_print_guid(guidbuffer, sizeof(guidbuffer), &(msg->guid));

  snprintf(buffer, buffer_size, "%u,%u,%u,0,%s,%u,%s,", msg->head, msg->class,
           msg->type, timebuffer, msg->hw_timestamp, guidbuffer);

  for (i = 0; (i < msg->data_length) && (i < 8); i++) {
    char temp[8];
    snprintf(temp, 7, "%u,", msg->data[i]);
    strncat(buffer, temp, buffer_size - 1);
  }
  buffer[strlen(buffer) - 1] = 0;
  strncat(buffer, "\n\r", buffer_size);

  return strlen(buffer);
}

// Parses 00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD:EE:FF like strings and
// checks for errors
// Individual octets have to be written in HEX and can be single or double char
int vscp_strtoguid(const char *input, vscp_guid_t *guid) {
  const char *ptr = input;
  char temp[3];
  vscp_guid_t local_guid;
  unsigned int value;
  int field = 0;

  if (input == NULL)
    return -1;

  while (*ptr != 0) {
    const char *seek = ptr;
    char guard;

    if (field > 15)
      return -1;

    while (*seek != ':' && *seek != 0)
      seek++;

    if ((seek - ptr) > 2)
      return -1;

    strncpy(temp, ptr, seek - ptr); /* no null terminator is copied */
    temp[seek - ptr] = 0;           /* so we do it! */

    if (sscanf(temp, "%X%c", &value, &guard) != 1)
      return -1;

    /* not really possible with only 2 chars but hey.. */
    if (value > UINT8_MAX)
      return -1;

    local_guid.guid[field] = (uint8_t)value;

    ptr = seek;
    if (*ptr != 0)
      ptr++;
    field++;
  }
  if (*ptr != 0 || field != 16)
    return -1;
  else {
    *guid = local_guid;
    return 0;
  }
}

int vscp_print_guid(char *buffer, size_t buffer_size, const vscp_guid_t *guid) {
  return snprintf(
      buffer, buffer_size, "%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X:%X",
      guid->guid[0], guid->guid[1], guid->guid[2], guid->guid[3], guid->guid[4],
      guid->guid[5], guid->guid[6], guid->guid[7], guid->guid[8], guid->guid[9],
      guid->guid[10], guid->guid[11], guid->guid[12], guid->guid[13],
      guid->guid[14], guid->guid[15]);
}
