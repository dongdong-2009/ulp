/*
*	miaofng@2012 initial version
*/
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/err.h"
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#include "ethernetif.h"
#include "ulp_time.h"
#include "lpc177x_8x_emac.h"
#include "lpc177x_8x_pinsel.h"
#include <string.h>

void low_level_init(struct netif *netif)
{
	EMAC_CFG_Type Emac_Config;

	/*
	 * Enable P1 Ethernet Pins:
	 * P1.0 - ENET_TXD0
	 * P1.1 - ENET_TXD1
	 * P1.4 - ENET_TX_EN
	 * P1.8 - ENET_CRS
	 * P1.9 - ENET_RXD0
	 * P1.10 - ENET_RXD1
	 * P1.14 - ENET_RX_ER
	 * P1.15 - ENET_REF_CLK
	 * P1.16 - ENET_MDC
	 * P1.17 - ENET_MDIO
	 */
	PINSEL_ConfigPin(1,0,1);
	PINSEL_ConfigPin(1,1,1);
	PINSEL_ConfigPin(1,4,1);
	PINSEL_ConfigPin(1,8,1);
	PINSEL_ConfigPin(1,9,1);
	PINSEL_ConfigPin(1,10,1);
	PINSEL_ConfigPin(1,14,1);
	PINSEL_ConfigPin(1,15,1);
	PINSEL_ConfigPin(1,16,1);
	PINSEL_ConfigPin(1,17,1);

	Emac_Config.Mode = EMAC_MODE_AUTO;
	//Emac_Config.Mode = EMAC_MODE_100M_FULL;
	//Emac_Config.Mode = EMAC_MODE_100M_HALF;
	//Emac_Config.Mode = EMAC_MODE_10M_FULL;
	//Emac_Config.Mode = EMAC_MODE_10M_HALF;
	Emac_Config.pbEMAC_Addr = netif->hwaddr;
	// Initialize EMAC module with given parameter
	if (EMAC_Init(&Emac_Config) == ERROR){
		return;
	}

	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	/* maximum transfer unit */
	netif->mtu = 1500;
	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	return;
}

/*
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *	 an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	EMAC_PACKETBUF_Type TxPack;
	struct pbuf *q;

	//write payload to buffer
	for (q = p; q != NULL; q = q->next) {
		if(q->len <= 0)
			continue;

		// check Tx Slot is available
		while (EMAC_CheckTransmitIndex() == FALSE);

		//size = MIN(size,EMAC_MAX_PACKET_SIZE);
		// Setup Tx Packet buffer
		TxPack.ulDataLen = q->len;
		TxPack.pbDataBuf = (uint32_t *)q->payload;
		EMAC_WritePacketBuffer(&TxPack);
		EMAC_UpdateTxProduceIndex();
	}
	return ERR_OK;
}

/*
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *	 NULL on memory error
 */
struct pbuf * low_level_input(struct netif *netif)
{
	EMAC_PACKETBUF_Type RxPack;
	struct pbuf *p, *q;
	int length;

	// Test if at least one packet has been received and is waiting
	if (EMAC_CheckReceiveIndex() == FALSE){
		return(NULL);
	}

	// fill pbuf
	length = EMAC_GetReceiveDataSize() + 1;
	p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
	if (p != NULL) {
		// Get size of receive data
		length = EMAC_GetReceiveDataSize() + 1;

		//Size = MIN(Size,in_size);

		// Setup Rx packet
		RxPack.pbDataBuf = (uint32_t *)p->payload;
		RxPack.ulDataLen = length;
		EMAC_ReadPacketBuffer(&RxPack);

		// update receive status
		EMAC_UpdateRxConsumeIndex();
	}
	return p;
}
