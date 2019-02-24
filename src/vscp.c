#include <stdio.h>
#include <string.h>

#include "vscp.h"

//          0    1      2   3     4         5       6     7     8     9
// parses "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
// datetime YYYY-MM-DDTHH:MM:DD
int vscp_parse_msg(const char *input, vscp_msg_t *msg) {
  const char *ptr = input;
  char *temp;
  unsigned int value;
  int field = 0;

  msg->data_length = 0;
  msg->origin = 0;

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

void vscp_to_can(const vscp_msg_t *msg, struct can_frame *frame) {
  int i;
  frame->can_id = CAN_EFF_FLAG | ((msg->head & 0xF0) << 21) | msg->class << 16 |
                  msg->type << 8 | msg->origin;
  frame->can_dlc = msg->data_length;
  for (i = 0; i < msg->data_length; i++)
    frame->data[i] = msg->data[i];
}

int can_to_vscp(const struct can_frame *frame,
                const struct timeval *hw_timestamp, vscp_msg_t *msg) {
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
  msg->origin = (uint8_t)((frame->can_id & 0x000000FFU));
  msg->data_length = frame->can_dlc;
  msg->timestamp = time(NULL);
  msg->hw_timestamp = (uint32_t)(hw_timestamp->tv_usec);

  for (i = 0; i < msg->data_length; i++) {
    msg->data[i] = frame->data[i];
  }
  return 0;
}
// "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
// datetime YYYY-MM-DDTHH:MM:DD
int print_vscp(const vscp_msg_t *msg, char *buffer, size_t buffer_size) {
  int i;

  snprintf(buffer, buffer_size - 1, "%u,%u,%u,0,XX,%u,%s%u,", msg->head,
           msg->class, msg->type, msg->hw_timestamp, "GUID:", msg->origin);

  for (i = 0; (i < msg->data_length) && (i < 8); i++) {
    char temp[8];
    snprintf(temp, 7, "%u,", msg->data[i]);
    strncat(buffer, temp, buffer_size - 1);
  }
  buffer[strlen(buffer)-1] = 0;
  strncat(buffer, "\n\r", buffer_size - 1);

  return strlen(buffer);
}
