/*
*	dusk@2010 initial version
*/
#ifndef __STM32_MAC_H_
#define __STM32_MAC_H_

#include "lwip/err.h"
#include "lwip/netif.h"

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

#endif /*__STM32_MAC_*/

