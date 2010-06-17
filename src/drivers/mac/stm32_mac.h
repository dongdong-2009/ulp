/*
*	dusk@2010 initial version
*/
#ifndef __STM32_MAC_H_
#define __STM32_MAC_H_

#include "lwip/err.h"
#include "lwip/netif.h"

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

#define ETH_RXBUFNB		4
#define ETH_TXBUFNB		2

#define  ETH_DMARxDesc_FrameLengthShift		16
#define  ETH_ERROR		((u32)0)
#define  ETH_SUCCESS 	((u32)1)

/* Private define */
#define DP83848_PHY
#define PHY_ADDRESS	0x01

//#define MII_MODE      /* MII mode for STM3210C-EVAL Board (MB784) (check jumpers setting) */
#define RMII_MODE       /* RMII mode for STM3210C-EVAL Board (MB784) (check jumpers setting) */

void stm32mac_Init(void);

err_t ethernetif_init(struct netif *netif);
err_t ethernetif_input(struct netif *netif);
struct netif *ethernetif_register(void);
int ethernetif_poll(void);
void Set_MAC_Address(unsigned char* macadd);

#ifdef SERVER
#define MAC_ADDR0 0x00
#define MAC_ADDR1 0x00
#define MAC_ADDR2 0x00
#define MAC_ADDR3 0x00
#define MAC_ADDR4 0x00
#define MAC_ADDR5 0x01
#else
#define MAC_ADDR0 0x00
#define MAC_ADDR1 0x00
#define MAC_ADDR2 0x00
#define MAC_ADDR3 0x00
#define MAC_ADDR4 0x00
#define MAC_ADDR5 0x03
#endif

#endif /*__STM32_MAC_*/

