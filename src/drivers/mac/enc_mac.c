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
#include "enc424j600.h"

#include "ethernetif.h"
#include "spi.h"
#include "ulp_time.h"
#include "stm32f10x.h"
#include <string.h>

/*================ Private functions =============================*/

#define CS_LOW() spi_cs_set(enc24j.idx, 0)
#define CS_HIGH() spi_cs_set(enc24j.idx, 1)

#define GP_WINDOW		(0x2)
#define RX_WINDOW		(0x4)

#define RXSTART 0x1000u
#define TXSTART 0x0000u

// Internal MAC level variables and flags.
unsigned char vCurrentBank=0xff;
int rCurrentPacketPointer;
int rNextPacketPointer;

enc_t enc24j = {
	.bus = &spi1,
	.idx = SPI_1_NSS,
};

void low_level_init(struct netif *netif)
{
	int w;
	enc_Init();
	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	/* set MAC hardware address */
	((unsigned char*)&w)[0] = netif->hwaddr[0];
	((unsigned char*)&w)[1] = netif->hwaddr[1];
	WriteReg(MAADR1, w);
	((unsigned char*)&w)[0] = netif->hwaddr[2];
	((unsigned char*)&w)[1] = netif->hwaddr[3];
	WriteReg(MAADR2, w);
	((unsigned char*)&w)[0] = netif->hwaddr[4];
	((unsigned char*)&w)[1] = netif->hwaddr[5];
	WriteReg(MAADR3,w);
	/* maximum transfer unit */
	netif->mtu = 1500;
	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	GPIO_InitTypeDef  GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode= GPIO_Mode_IN_FLOATING ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
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
	int addr, busy;
	struct pbuf *q;
	addr = ReadReg(ETXST);
	if(ReadReg(ECON1) & ECON1_TXRTS) {
		addr = (addr == 0x800) ? 0 : 0x800;
		busy = 1;
	}
	//write payload to buffer
	WriteReg(EGPWRPT, addr);
	for (q = p; q != NULL; q = q->next) {
		WriteMemoryWindow(GP_WINDOW, q->payload, q->len);
	}
	if(!(ReadReg(ESTAT) & ESTAT_PHYLNK))
		return ERR_CONN;
	//wait old packet sent
	while(busy && (ReadReg(ECON1) & ECON1_TXRTS));
	
	//send current packet
	WriteReg(ETXST, addr);
	WriteReg(ETXLEN, p -> len);
	BFSReg(ECON1, ECON1_TXRTS);
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
	struct pbuf *p, *q;
	int length;
	struct enc_head_s header;
	// Test if at least one packet has been received and is waiting
	if(!(ReadReg(EIR) & EIR_PKTIF)) {
		return(NULL);
	}

	// Set the RX Read Pointer to the beginning of the next unprocessed packet
	rCurrentPacketPointer = rNextPacketPointer;
	WriteReg(ERXRDPT, rCurrentPacketPointer);
	ReadMemoryWindow(RX_WINDOW, (unsigned char *) &header, sizeof(header));
	rNextPacketPointer = header.next; 
	length = header.n; 
	p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
	if (p != NULL) {
		for (q = p; q != NULL; q = q -> next) {
			ReadMemoryWindow(RX_WINDOW, q -> payload, q -> len);
			
		}
	}
	WriteReg(ERXTAIL, rNextPacketPointer-2);
	BFSReg(ECON1, ECON1_PKTDEC);
	return p;
}

void enc_Init(void)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.csel = 1,
		.freq = 9000000,
	};
	enc24j.bus->init(&cfg);
	SendSystemReset();
	rNextPacketPointer = RXSTART;
	rCurrentPacketPointer = 0x0000;

	// Set up TX/RX/UDA buffer addresses
	WriteReg(ETXST, TXSTART);
	WriteReg(ERXST, RXSTART);
	WriteReg(ERXTAIL, ENC100_RAM_SIZE-2);
	WriteReg(EUDAST, ENC100_RAM_SIZE);
	WriteReg(EUDAND, ENC100_RAM_SIZE+1);
	WritePHYReg(PHANA, PHANA_ADPAUS0 | PHANA_AD10FD | PHANA_AD10 | PHANA_AD100FD | PHANA_AD100 | PHANA_ADIEEE0);
	WritePHYReg(PHCON1, PHCON1_SPD100 | PHCON1_PFULDPX);

	//Enable interupt
	BFSReg(EIE, EIE_INTIE);
	BFSReg(EIE, EIE_PKTIE);

	// Enable RX packet reception
	BFSReg(ECON1, ECON1_RXEN);
}

void SendSystemReset(void)
{
	do {
		do {
			WriteReg(EUDAST, 0x1234);
		} while(ReadReg(EUDAST) != 0x1234u);
		// Issue a reset and wait for it to complete
		BFSReg(ECON2, ECON2_ETHRST);
		vCurrentBank = 0;
		while((ReadReg(ESTAT) & (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY)) != (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY));
		udelay(30);

	 // Check to see if the reset operation was successful by
	 // checking if EUDAST went back to its reset default.  This test
	// should always pass, but certain special conditions might make
	// this test fail, such as a PSP pin shorted to logic high.
	} while(ReadReg(EUDAST) != 0x0000u);
	// Really ensure reset is done and give some time for power to be stable
	mdelay(1);
}

void WritePHYReg(unsigned char Register, int Data)
{
	// Write the register address
	WriteReg(MIREGADR,  0x0100 | Register);

	// Write the data
	WriteReg(MIWR, Data);

	// Wait until the PHY register has been written
	while(ReadReg(MISTAT) & MISTAT_BUSY);
}

void BFSReg(int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				enc24j.bus->wreg(enc24j.idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				enc24j.bus->wreg(enc24j.idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				enc24j.bus->wreg(enc24j.idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				enc24j.bus->wreg(enc24j.idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		enc24j.bus->wreg(enc24j.idx, BFS | (wAddress & 0x1F));
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[0]);
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[1]);
	}
	CS_HIGH();
}

void BFCReg(int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				enc24j.bus->wreg(enc24j.idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				enc24j.bus->wreg(enc24j.idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				enc24j.bus->wreg(enc24j.idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				enc24j.bus->wreg(enc24j.idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		enc24j.bus->wreg(enc24j.idx, BFC | (wAddress & 0x1F));
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[0]);
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[1]);
	}
	CS_HIGH();

}

void WriteReg(int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				enc24j.bus->wreg(enc24j.idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				enc24j.bus->wreg(enc24j.idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				enc24j.bus->wreg(enc24j.idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				enc24j.bus->wreg(enc24j.idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		enc24j.bus->wreg(enc24j.idx, WCR | (wAddress & 0x1F));
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[0]);
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[1]);
	}
	else
	{
		enc24j.bus->wreg(enc24j.idx, WCRU);
		enc24j.bus->wreg(enc24j.idx, (unsigned char)wAddress);
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[0]);
		enc24j.bus->wreg(enc24j.idx, ((unsigned char*)&wValue)[1]);
		//Execute3(WCRU, dw);
	}
	CS_HIGH();
}

int ReadReg(int wAddress)
{
	unsigned int w;
	unsigned char vBank;
	CS_LOW();
	// See if we need to change register banks
	vBank = ((unsigned char)wAddress) & 0xE0;
	if(vBank <= (0x3u<<5)) {
		 if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				enc24j.bus->wreg(enc24j.idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				enc24j.bus->wreg(enc24j.idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				enc24j.bus->wreg(enc24j.idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				enc24j.bus->wreg(enc24j.idx, B3SEL);
			vCurrentBank = vBank;
		}

		//w = Execute2(RCR1 | (wAddress & 0x1F), 0x0000);
		enc24j.bus->wreg(enc24j.idx, RCR1 | (wAddress & 0x1F));
		((unsigned char*)&w)[0]=enc24j.bus->rreg(enc24j.idx);
		((unsigned char*)&w)[1]=enc24j.bus->rreg(enc24j.idx);
	}
	else {
		//long dw = Execute3(RCR1U, (long)wAddress);
		enc24j.bus->wreg(enc24j.idx, RCRU);
		enc24j.bus->rreg(enc24j.idx);
		((unsigned char*)&w)[0]=enc24j.bus->rreg(enc24j.idx);
		((unsigned char*)&w)[1]=enc24j.bus->rreg(enc24j.idx);
	}
	CS_HIGH();
	return w;
}

void ReadN(unsigned char vOpcode, unsigned char* vData, int wDataLen)
{
	CS_LOW();
	enc24j.bus->wreg(enc24j.idx,vOpcode);
	while(wDataLen--)
	{
		*vData++=enc24j.bus->rreg(enc24j.idx);
	}
	CS_HIGH();
}
void WriteN(unsigned char vOpcode, unsigned char* vData, int wDataLen)
{
	CS_LOW();
	enc24j.bus->wreg(enc24j.idx,vOpcode);
	while(wDataLen--)
	{
		enc24j.bus->wreg(enc24j.idx,*vData++);
	}
	CS_HIGH();
}

void WriteMemoryWindow(unsigned char vWindow, unsigned char *vData, int wLength)
{
	unsigned char vOpcode;
	vOpcode = WBMUDA;
	if(vWindow & GP_WINDOW)
		vOpcode = WBMGP;
	if(vWindow & RX_WINDOW)
		vOpcode = WBMRX;
	WriteN(vOpcode, vData, wLength);
}

void ReadMemoryWindow(unsigned char vWindow, unsigned char *vData,int wLength)
{
	unsigned char vOpcode;
	vOpcode = RBMUDA;
	if(vWindow & GP_WINDOW)
		vOpcode = RBMGP;
	if(vWindow & RX_WINDOW)
		vOpcode = RBMRX;
	ReadN(vOpcode, vData, wLength);
}
