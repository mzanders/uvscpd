#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <stdint.h>

  /* start a TCP server */
  void tcpserver_start (const char * can_bus, uint32_t ip_addr, uint16_t port) ;
  void tcpserver_stop (void);


#endif /* #ifndef _TCPSERVER_H_ */
