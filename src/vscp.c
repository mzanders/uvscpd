#include "vscp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//          0    1      2   3     4         5       6     7     8     9
// parses "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
// datetime YYYY-MM-DDTHH:MM:DD
int vscp_parse_msg(const char *input, vscp_msg_t *msg) {
  const char *ptr = input;
  char *temp;
  unsigned int value;
  int field = 0;

  msg->data_length = 0;

  while (*ptr != 0) {

    const char *seek = ptr;
    char guard;

    while (*seek != ',' && *seek != 0)
      seek++;

    temp = strndup(ptr, seek - ptr);

    switch (field) {
    case 0 ... 2:
    case 7 ... 14:
      if (sscanf(temp, "%u%c", &value, &guard) != 1)
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
      else
        return -1;
      break;

    case 2:
      if (value <= UINT8_MAX)
        msg->type = (uint8_t)value;
      else
        return -1;
      break;

    case 7 ... 14:
      if (value <= UINT8_MAX)
        msg->data[field - 7] = (uint8_t)value;
      else
        return -1;
      msg->data_length++;
      break;

    case 15:
      return -1;
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

void vscp_to_can(vscp_msg_t *msg, struct can_frame *frame) {
  int i;
  frame->can_id = CAN_EFF_FLAG | ((msg->head & 0xF0) << 20) | msg->class << 16 |
                  msg->type << 8;
  frame->can_dlc = msg->data_length;
  for (i = 0; i < msg->data_length; i++)
    frame->data[i] = msg->data[i];
}
