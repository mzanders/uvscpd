#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <stdint.h>


  /* The main controller */
  void controller (const char * can_bus, uint32_t ip_addr, uint16_t port);

#endif /* #ifndef _CONTROLLER_H_ */
