/**
* this file is used to provide lwip lib i/f func, lwip_xxx
*
* miaofng@2012 initial version
*
*/

#ifndef __ULP_LWIP_H_
#define __ULP_LWIP_H_

#include "config.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

//demo apps
void HelloWorld_init(void); //23
void tcpserver_Init(void); //port 3838

#endif /*__ULP_LWIP_H_*/
