#ifndef _VSCP_H_
#define _VSCP_H_

#include <linux/can/raw.h>
#include <stdint.h>

typedef struct
{
  uint8_t head;
  uint16_t class;
  uint8_t type;
  uint8_t data_length;
  uint8_t data[8];
} vscp_msg_t;

  // parses "head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3.."
  int vscp_parse_msg(const char * input, vscp_msg_t * msg);
  void vscp_to_can(vscp_msg_t * msg, struct can_frame * frame);
#endif /* #ifndef _VSCP_H_ */
