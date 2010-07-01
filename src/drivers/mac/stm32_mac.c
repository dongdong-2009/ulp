/*
*	dusk@2010 initial version
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
#include "stm32_mac.h"
#include "stm32_eth.h"
#include <string.h>

/* TCP and ARP timeouts */
volatile int tcp_end_time, arp_end_time;

typedef struct{
u32 length;
u32 buffer;
ETH_DMADESCTypeDef *descriptor;
}FrameTypeDef;

extern ETH_DMADESCTypeDef  *DMATxDescToSet;
extern ETH_DMADESCTypeDef  *DMARxDescToGet;

/*================ Private varibles =============================*/
/* Ethernet Rx & Tx DMA Descriptors */
ETH_DMADESCTypeDef  DMARxDscrTab[ETH_RXBUFNB], DMATxDscrTab[ETH_TXBUFNB];
/* Ethernet buffers */
uint8_t Rx_Buff[ETH_RXBUFNB][ETH_MAX_PACKET_SIZE], Tx_Buff[ETH_TXBUFNB][ETH_MAX_PACKET_SIZE];

ETH_DMADESCTypeDef  *DMATxDesc = DMATxDscrTab;

/*================ Private functions =============================*/
static void mac_GPIO_Configuration(void);
static void mac_NVIC_Configuration(void);
static void mac_Ethernet_Configuration(void);

static FrameTypeDef ETH_RxPkt_ChainMode(void);
static u32 ETH_GetCurrentTxBuffer(void);
static u32 ETH_TxPkt_ChainMode(u16 FrameLength);

void stm32mac_Init(void)
{
	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	/* Enable ETHERNET clock  */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC | RCC_AHBPeriph_ETH_MAC_Tx |
							RCC_AHBPeriph_ETH_MAC_Rx, ENABLE);
	/* Enable GPIOs and ADC1 clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
							RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO , ENABLE);
  
	/* NVIC configuration */
	mac_NVIC_Configuration();
	mac_GPIO_Configuration();
	mac_Ethernet_Configuration();
}

/*
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
void low_level_init(struct netif *netif)
{	
	stm32mac_Init();
	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	/* set MAC hardware address */
	netif->hwaddr[0] =  MAC_ADDR_BYTE0;
	netif->hwaddr[1] =  MAC_ADDR_BYTE1;
	netif->hwaddr[2] =  MAC_ADDR_BYTE2;
	netif->hwaddr[3] =  MAC_ADDR_BYTE3;
	netif->hwaddr[4] =  MAC_ADDR_BYTE4;
	netif->hwaddr[5] =  MAC_ADDR_BYTE5;
	ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);

	/* maximum transfer unit */
	netif->mtu = 1500;

	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	/* Initialize Tx Descriptors list: Chain Mode */
	ETH_DMATxDescChainInit(DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);
	/* Initialize Rx Descriptors list: Chain Mode  */
	ETH_DMARxDescChainInit(DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

	/* Enable Ethernet Rx interrrupt */
	{ int i;
	for(i=0; i<ETH_RXBUFNB; i++) {
		ETH_DMARxDescReceiveITConfig(&DMARxDscrTab[i], ENABLE);
	}
	}

#ifdef CHECKSUM_BY_HARDWARE
	/* Enable the checksum insertion for the Tx frames */
	{ int i;
	for(i=0; i<ETH_TXBUFNB; i++) {
	ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i], ETH_DMATxDesc_ChecksumTCPUDPICMPFull);
	}
	}
#endif

	/* Enable MAC and DMA transmission and reception */
	ETH_Start();
}

/*
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	int l = 0;
	u8 *buffer =  (u8 *)ETH_GetCurrentTxBuffer();
  
	for (q = p; q != NULL; q = q->next) {
		memcpy((u8_t*)&buffer[l], q->payload, q->len);
		l = l + q->len;
	}

	ETH_TxPkt_ChainMode(l);

	return ERR_OK;
}

/*
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
struct pbuf * low_level_input(struct netif *netif)
{
	struct pbuf *p, *q;
	u16_t len;
	int l =0;
	FrameTypeDef frame;
	u8 *buffer;
  
	p = NULL;
	frame = ETH_RxPkt_ChainMode();
	len = frame.length;
  
	buffer = (u8 *)frame.buffer;

	/* We allocate a pbuf chain of pbufs from the pool. */
	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

	if (p != NULL) {
		for (q = p; q != NULL; q = q->next)	{
			memcpy((u8_t*)q->payload, (u8_t*)&buffer[l], q->len);
			l = l + q->len;
		}
	}

	/* Set Own bit of the Rx descriptor Status: gives the buffer back to ETHERNET DMA */
	frame.descriptor->Status = ETH_DMARxDesc_OWN; 
 
	/* When Rx Buffer unavailable flag is set: clear it and resume reception */
	if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		ETH->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		ETH->DMARPDR = 0;
	}

	return p;
}

/*
 * Function Name  : ETH_RxPkt_ChainMode
 * Description    : Receives a packet.
 * Return         : frame: farme size and location
 */
static FrameTypeDef ETH_RxPkt_ChainMode(void)
{ 
	u32 framelength = 0;
	FrameTypeDef frame = {0,0}; 

	/* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
	if((DMARxDescToGet->Status & ETH_DMARxDesc_OWN) != (u32)RESET) {	
		frame.length = ETH_ERROR;

		if ((ETH->DMASR & ETH_DMASR_RBUS) != (u32)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		ETH->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		ETH->DMARPDR = 0;
		}

		/* Return error: OWN bit set */
		return frame; 
	}
  
	if(((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == (u32)RESET) && 
		((DMARxDescToGet->Status & ETH_DMARxDesc_LS) != (u32)RESET) &&  
		((DMARxDescToGet->Status & ETH_DMARxDesc_FS) != (u32)RESET)) {      
		/* Get the Frame Length of the received packet: substruct 4 bytes of the CRC */
		framelength = ((DMARxDescToGet->Status & ETH_DMARxDesc_FL) >> ETH_DMARxDesc_FrameLengthShift) - 4;
	
		/* Get the addrees of the actual buffer */
		frame.buffer = DMARxDescToGet->Buffer1Addr;	
	}  else {
		framelength = ETH_ERROR;
	}

	frame.length = framelength;
	frame.descriptor = DMARxDescToGet;
  
	/* Update the ETHERNET DMA global Rx descriptor with next Rx decriptor */      
	/* Chained Mode */    
	/* Selects the next DMA Rx descriptor list for next buffer to read */ 
	DMARxDescToGet = (ETH_DMADESCTypeDef*) (DMARxDescToGet->Buffer2NextDescAddr);    

	return (frame);  
}

/*
 * Function Name  : ETH_TxPkt_ChainMode
 * Description    : Transmits a packet, from application buffer, pointed by ppkt.
 * Input          : - FrameLength: Tx Packet size.
 * Return         : ETH_ERROR: in case of Tx desc owned by DMA
 *                  ETH_SUCCESS: for correct transmission
 */
static u32 ETH_TxPkt_ChainMode(u16 FrameLength)
{   
	/* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
	if((DMATxDescToSet->Status & ETH_DMATxDesc_OWN) != (u32)RESET)
		return ETH_ERROR;
        
	/* Setting the Frame Length: bits[12:0] */
	DMATxDescToSet->ControlBufferSize = (FrameLength & ETH_DMATxDesc_TBS1);

	/* Setting the last segment and first segment bits (in this case a frame is transmitted in one descriptor) */    
	DMATxDescToSet->Status |= ETH_DMATxDesc_LS | ETH_DMATxDesc_FS;

	/* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
	DMATxDescToSet->Status |= ETH_DMATxDesc_OWN;

	/* When Tx Buffer unavailable flag is set: clear it and resume transmission */
	if ((ETH->DMASR & ETH_DMASR_TBUS) != (u32)RESET) {
		/* Clear TBUS ETHERNET DMA flag */
		ETH->DMASR = ETH_DMASR_TBUS;
		/* Resume DMA transmission*/
		ETH->DMATPDR = 0;
	}
  
	/* Update the ETHERNET DMA global Tx descriptor with next Tx decriptor */  
	/* Chained Mode */
	/* Selects the next DMA Tx descriptor list for next buffer to send */ 
	DMATxDescToSet = (ETH_DMADESCTypeDef*) (DMATxDescToSet->Buffer2NextDescAddr);    

	return ETH_SUCCESS;
}

/*
 * Function Name  : ETH_GetCurrentTxBuffer
 * Description    : Return the address of the buffer pointed by the current descritor.
 * Return         : Buffer address
 */
static u32 ETH_GetCurrentTxBuffer(void)
{ 
  /* Return Buffer address */
  return (DMATxDescToSet->Buffer1Addr);   
}

static void mac_Ethernet_Configuration(void)
{
	ETH_InitTypeDef ETH_InitStructure;

	/* MII/RMII Media interface selection */
#ifdef MII_MODE /* Mode MII with STM3210C-EVAL  */
	GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_MII);
	
	/* Get HSE clock = 25MHz on PA8 pin (MCO) */
	RCC_MCOConfig(RCC_MCO_HSE);
#elif defined RMII_MODE  /* Mode RMII with STM3210C-EVAL */
	GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_RMII);

	/* Set PLL3 clock output to 50MHz (25MHz /5 *10 =50MHz) */
	RCC_PLL3Config(RCC_PLL3Mul_10);
	RCC_PLL3Cmd(ENABLE);
	while (RCC_GetFlagStatus(RCC_FLAG_PLL3RDY) == RESET);
	/* Get PLL3 clock on PA8 pin (MCO) */
	RCC_MCOConfig(RCC_MCO_PLL3CLK);
#endif
	ETH_DeInit();
	ETH_SoftwareReset();
	while (ETH_GetSoftwareResetStatus() == SET);

	ETH_StructInit(&ETH_InitStructure);
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable  ;
	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Disable;
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
#endif

	/*------------------------   DMA   -----------------------------------*/   
	/* When we use the Checksum offload feature, we need to enable the Store and Forward mode: 
	the store and forward guarantee that a whole frame is stored in the FIFO, 
	so the MAC can insert/verify the checksum, 
	if the checksum is OK the DMA can handle the frame otherwise the frame is dropped */
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; 
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;         
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;     
 
	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;       
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;   
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;                                                          
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;      
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;                
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;          
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;                                                                 
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;

	ETH_Init(&ETH_InitStructure, PHY_ADDRESS);

	/* Enable the Ethernet Rx Interrupt */
	ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
}

/*
 * @brief  Configures the different GPIO ports.
 */
static void mac_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* ETHERNET pins configuration */
	/* AF Output Push Pull:
	- ETH_MII_MDIO / ETH_RMII_MDIO: PA2
	- ETH_MII_MDC / ETH_RMII_MDC: PC1
	- ETH_MII_TXD2: PC2
	- ETH_MII_TX_EN / ETH_RMII_TX_EN: PB11
	- ETH_MII_TXD0 / ETH_RMII_TXD0: PB12
	- ETH_MII_TXD1 / ETH_RMII_TXD1: PB13
	- ETH_MII_PPS_OUT / ETH_RMII_PPS_OUT: PB5
	- ETH_MII_TXD3: PB8 */

	/* Configure PA2 as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure PC1, PC2 and PC3 as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure PB5, PB8, PB11, PB12 and PB13 as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_11 |
									GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/**************************************************************/
	/*               For Remapped Ethernet pins                   */
	/*************************************************************/
	/* Input (Reset Value):
	- ETH_MII_CRS CRS: PA0
	- ETH_MII_RX_CLK / ETH_RMII_REF_CLK: PA1
	- ETH_MII_COL: PA3
	- ETH_MII_RX_DV / ETH_RMII_CRS_DV: PD8
	- ETH_MII_TX_CLK: PC3
	- ETH_MII_RXD0 / ETH_RMII_RXD0: PD9
	- ETH_MII_RXD1 / ETH_RMII_RXD1: PD10
	- ETH_MII_RXD2: PD11
	- ETH_MII_RXD3: PD12
	- ETH_MII_RX_ER: PB10 */

	/* ETHERNET pins remapp in STM3210C-EVAL board: RX_DV and RxD[3:0] */
	GPIO_PinRemapConfig(GPIO_Remap_ETH, ENABLE);

	/* Configure PA0, PA1 and PA3 as input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure PB10 as input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure PC3 as input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure PD8, PD9, PD10, PD11 and PD12 as input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure); /**/

	/* ADC Channel14 config --------------------------------------------------------*/
	/* Relative to STM3210D-EVAL Board   */
	/* Configure PC.04 (ADC Channel14) as analog input -------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* MCO pin configuration------------------------------------------------- */
	/* Configure MCO (PA8) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*
 * @brief  Configures the nested vectored interrupt controller.
 */
static void mac_NVIC_Configuration(void)
{
	NVIC_InitTypeDef   NVIC_InitStructure;

	/* Set the Vector Table base location at 0x08000000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

	/* 2 bit for pre-emption priority, 2 bits for subpriority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
  
	/* Enable the Ethernet global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  
}
