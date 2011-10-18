/*
 * 	dusk@2010 initial version
 */
#include "eth_demo/helloworld.h"
#include "eth_demo/tcpserver.h"
#include "lwipopts.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include <stdio.h>
#include "ulp_time.h"
#include "sys/task.h"

//this function is global and will be called by lwip_lib.c
void lwip_app_Init(void)
{
	//Initialize the HelloWorld module
	HelloWorld_init();
	//Initialize the TCP server
	tcpserver_Init();
}
