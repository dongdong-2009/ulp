/*
 * 	dusk@2010 initial version
 */
#include "eth_demo/eth_demo.h"
#include "lwipopts.h"
#include "eth_demo/helloworld.h"
#include "eth_demo/tcpserver.h"

#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"

#include "ethernetif.h"
#include <stdio.h>
#include "time.h"
#include "sys/task.h"

/* Private typedef */
#define MAX_DHCP_TRIES        4
#define SELECTED              1
#define NOT_SELECTED		  (!SELECTED)
#define CLIENTMAC6            2
//#define CLIENTMAC6            3
//#define CLIENTMAC6            4

/* Private variables */
struct netif netif;
static unsigned TCPTimer = 0;
static unsigned ARPTimer = 0;

#if LWIP_DHCP == 1
static unsigned DHCPfineTimer = 0;
static unsigned DHCPcoarseTimer = 0;
static unsigned IPaddress = 0;
#endif

static unsigned LocalTime = 0;
static time_t lwip_timer;

/* public function prototypes */
static void LwIP_Periodic_Handle(unsigned localtime);

void eth_demo_Init(void)
{
	struct ip_addr ipaddr;
	struct ip_addr netmask;
	struct ip_addr gw;

	/* Initializes the dynamic memory heap defined by MEM_SIZE.*/
	mem_init();
	/* Initializes the memory pools defined by MEMP_NUM_x.*/
	memp_init();

#if LWIP_DHCP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else
	IP4_ADDR(&ipaddr, 192, 168, 1, 99);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	IP4_ADDR(&gw, 192, 168, 1, 254);
#endif

	netif_add(&netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &ethernet_input);
	netif_set_default(&netif);

#if LWIP_DHCP
	dhcp_start(&netif);
#endif

	/*  When the netif is fully configured this function must be called.*/
	netif_set_up(&netif);

	lwip_timer = time_get(SYSTEMTICK_PERIOD_MS);
	
	/* Initilaize the HelloWorld module */
	HelloWorld_init();
	
	tcpserver_Init();
}

void eth_demo_Update(void)
{
	if (time_left(lwip_timer) < 0) {
		lwip_timer = time_get(SYSTEMTICK_PERIOD_MS);
		LocalTime += SYSTEMTICK_PERIOD_MS;
		LwIP_Periodic_Handle(LocalTime);
	}
}

DECLARE_TASK(eth_demo_Init, eth_demo_Update)

void eth_demo_isr(void)
{
	ethernetif_input(&netif);
}

/*============ private functions ======================*/
static void LwIP_Periodic_Handle(unsigned localtime)
{
	/* TCP periodic process every 250 ms */
	if (localtime - TCPTimer >= TCP_TMR_INTERVAL) {
		TCPTimer =  localtime;
		tcp_tmr();
	}
	/* ARP periodic process every 5s */
	if (localtime - ARPTimer >= ARP_TMR_INTERVAL) {
		ARPTimer =  localtime;
		etharp_tmr();
	}

#if LWIP_DHCP
	/* Fine DHCP periodic process every 500ms */
	if (localtime - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS)	{
		DHCPfineTimer =  localtime;
		dhcp_fine_tmr();
	}
	/* DHCP Coarse periodic process every 60s */
	if (localtime - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS)	{
		DHCPcoarseTimer =  localtime;
		dhcp_coarse_tmr();
	}
#endif
}