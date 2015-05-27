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
#include "time.h"
#include "stm32f10x.h"
#include <string.h>

/*================ Private functions =============================*/

#define CS_LOW() spi_cs_set(chip->idx, 0)
#define CS_HIGH() spi_cs_set(chip->idx, 1)

#define GP_WINDOW		(0x2)
#define RX_WINDOW		(0x4)

#define RXSTART 0x1000u
#define TXSTART 0x0000u

// Internal MAC level variables and flags.
unsigned char vCurrentBank=0xff;
int rCurrentPacketPointer;
int rNextPacketPointer;

enc_t chip = {
	.bus = &spi1,
	.idx = SPI_1_NSS,
};

void low_level_init(struct netif *netif)
{
	int w;
	enc_Init(&chip);
	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;
	/* set MAC hardware address */
	((unsigned char*)&w)[0] = netif->hwaddr[0];
	((unsigned char*)&w)[1] = netif->hwaddr[1];
	WriteReg(&chip,MAADR1, w);
	((unsigned char*)&w)[0] = netif->hwaddr[2];
	((unsigned char*)&w)[1] = netif->hwaddr[3];
	WriteReg(&chip,MAADR2, w);
	((unsigned char*)&w)[0] = netif->hwaddr[4];
	((unsigned char*)&w)[1] = netif->hwaddr[5];
	WriteReg(&chip,MAADR3,w);
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
	addr = ReadReg(&chip, ETXST);
	if(ReadReg(&chip, ECON1) & ECON1_TXRTS) {
		addr = (addr == 0x800) ? 0 : 0x800;
		busy = 1;
	}
	//write payload to buffer
	WriteReg(&chip, EGPWRPT, addr);
	for (q = p; q != NULL; q = q->next) {
		WriteMemoryWindow(&chip,GP_WINDOW, q->payload, q->len);
	}
	if(!(ReadReg(&chip,ESTAT) & ESTAT_PHYLNK))
		return ERR_CONN;
	//wait old packet sent
	while(busy && (ReadReg(&chip, ECON1) & ECON1_TXRTS));
	
	//send current packet
	WriteReg(&chip, ETXST, addr);
	WriteReg(&chip, ETXLEN, p -> len);
	BFSReg(&chip,ECON1, ECON1_TXRTS);
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
	if(!(ReadReg(&chip,EIR) & EIR_PKTIF)) {
		return(NULL);
	}

	// Set the RX Read Pointer to the beginning of the next unprocessed packet
	rCurrentPacketPointer = rNextPacketPointer;
	WriteReg(&chip,ERXRDPT, rCurrentPacketPointer);
	ReadMemoryWindow(&chip,RX_WINDOW, (unsigned char *) &header, sizeof(header));
	rNextPacketPointer = header.next; 
	length = header.n; 
	p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
	if (p != NULL) {
		for (q = p; q != NULL; q = q -> next) {
			ReadMemoryWindow(&chip, RX_WINDOW, q -> payload, q -> len);
			
		}
	}
	WriteReg(&chip,ERXTAIL, rNextPacketPointer-2);
	BFSReg(&chip,ECON1, ECON1_PKTDEC);
	return p;
}

void enc_Init(enc_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.csel = 1,
		.freq = 9000000,
	};
	chip->bus->init(&cfg);
	SendSystemReset(*chip);
	rNextPacketPointer = RXSTART;
	rCurrentPacketPointer = 0x0000;

	// Set up TX/RX/UDA buffer addresses
	WriteReg(chip,ETXST, TXSTART);
	WriteReg(chip,ERXST, RXSTART);
	WriteReg(chip,ERXTAIL, ENC100_RAM_SIZE-2);
	WriteReg(chip,EUDAST, ENC100_RAM_SIZE);
	WriteReg(chip,EUDAND, ENC100_RAM_SIZE+1);
	WritePHYReg(*chip,PHANA, PHANA_ADPAUS0 | PHANA_AD10FD | PHANA_AD10 | PHANA_AD100FD | PHANA_AD100 | PHANA_ADIEEE0);
	WritePHYReg(*chip,PHCON1, PHCON1_SPD100 | PHCON1_PFULDPX);

	//Enable interupt
	BFSReg(chip,EIE, EIE_INTIE);
	BFSReg(chip,EIE, EIE_PKTIE);

	// Enable RX packet reception
	BFSReg(chip,ECON1, ECON1_RXEN);
}

void SendSystemReset(enc_t chip)
{
	do {
		do {
			WriteReg(&chip,EUDAST, 0x1234);
		} while(ReadReg(&chip,EUDAST) != 0x1234u);
		// Issue a reset and wait for it to complete
		BFSReg(&chip,ECON2, ECON2_ETHRST);
		vCurrentBank = 0;
		while((ReadReg(&chip,ESTAT) & (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY)) != (ESTAT_CLKRDY | ESTAT_RSTDONE | ESTAT_PHYRDY));
		udelay(30);

	 // Check to see if the reset operation was successful by
	 // checking if EUDAST went back to its reset default.  This test
	// should always pass, but certain special conditions might make
	// this test fail, such as a PSP pin shorted to logic high.
	} while(ReadReg(&chip,EUDAST) != 0x0000u);
	// Really ensure reset is done and give some time for power to be stable
	mdelay(1);
}

void WritePHYReg(enc_t chip,unsigned char Register, int Data)
{
	// Write the register address
	WriteReg(&chip,MIREGADR,  0x0100 | Register);

	// Write the data
	WriteReg(&chip,MIWR, Data);

	// Wait until the PHY register has been written
	while(ReadReg(&chip,MISTAT) & MISTAT_BUSY);
}

void BFSReg(enc_t *chip,int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				chip->bus->wreg(chip->idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				chip->bus->wreg(chip->idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				chip->bus->wreg(chip->idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				chip->bus->wreg(chip->idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		chip->bus->wreg(chip->idx, BFS | (wAddress & 0x1F));
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[0]);
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[1]);
	}
	CS_HIGH();
}

void BFCReg(enc_t *chip,int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				chip->bus->wreg(chip->idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				chip->bus->wreg(chip->idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				chip->bus->wreg(chip->idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				chip->bus->wreg(chip->idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		chip->bus->wreg(chip->idx, BFC | (wAddress & 0x1F));
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[0]);
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[1]);
	}
	CS_HIGH();

}

void WriteReg(enc_t *chip,int wAddress, int wValue)
{
	unsigned char vBank;
	vBank = ((unsigned char)wAddress) & 0xE0;
	CS_LOW();
	if(vBank <= (0x3u<<5)) {
		if(vBank != vCurrentBank) {
			if(vBank == (0x0u<<5))
				//Execute0(B0SEL);
				chip->bus->wreg(chip->idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				chip->bus->wreg(chip->idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				chip->bus->wreg(chip->idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				chip->bus->wreg(chip->idx, B3SEL);
			vCurrentBank = vBank;
		}

		//Execute2(WCR | (wAddress & 0x1F), wValue);
		chip->bus->wreg(chip->idx, WCR | (wAddress & 0x1F));
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[0]);
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[1]);
	}
	else
	{
		chip->bus->wreg(chip->idx, WCRU);
		chip->bus->wreg(chip->idx, (unsigned char)wAddress);
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[0]);
		chip->bus->wreg(chip->idx, ((unsigned char*)&wValue)[1]);
		//Execute3(WCRU, dw);
	}
	CS_HIGH();
}

int ReadReg(enc_t *chip,int wAddress)
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
				chip->bus->wreg(chip->idx, B0SEL);
			else if(vBank == (0x1u<<5))
				//Execute0(B1SEL);
				chip->bus->wreg(chip->idx, B1SEL);
			else if(vBank == (0x2u<<5))
				//Execute0(B2SEL);
				chip->bus->wreg(chip->idx, B2SEL);
			else if(vBank == (0x3u<<5))
				//Execute0(B3SEL);
				chip->bus->wreg(chip->idx, B3SEL);
			vCurrentBank = vBank;
		}

		//w = Execute2(RCR1 | (wAddress & 0x1F), 0x0000);
		chip->bus->wreg(chip->idx, RCR1 | (wAddress & 0x1F));
		((unsigned char*)&w)[0]=chip->bus->rreg(chip->idx);
		((unsigned char*)&w)[1]=chip->bus->rreg(chip->idx);
	}
	else {
		//long dw = Execute3(RCR1U, (long)wAddress);
		chip->bus->wreg(chip->idx, RCRU);
		chip->bus->rreg(chip->idx);
		((unsigned char*)&w)[0]=chip->bus->rreg(chip->idx);
		((unsigned char*)&w)[1]=chip->bus->rreg(chip->idx);
	}
	CS_HIGH();
	return w;
}

void ReadN(enc_t *chip,unsigned char vOpcode, unsigned char* vData, int wDataLen)
{
	CS_LOW();
	chip->bus->wreg(chip->idx,vOpcode);
	while(wDataLen--)
	{
		*vData++=chip->bus->rreg(chip->idx);
	}
	CS_HIGH();
}
void WriteN(enc_t *chip,unsigned char vOpcode, unsigned char* vData, int wDataLen)
{
	CS_LOW();
	chip->bus->wreg(chip->idx,vOpcode);
	while(wDataLen--)
	{
		chip->bus->wreg(chip->idx,*vData++);
	}
	CS_HIGH();
}

void WriteMemoryWindow(enc_t *chip,unsigned char vWindow, unsigned char *vData, int wLength)
{
	unsigned char vOpcode;
	vOpcode = WBMUDA;
	if(vWindow & GP_WINDOW)
		vOpcode = WBMGP;
	if(vWindow & RX_WINDOW)
		vOpcode = WBMRX;
	WriteN(chip,vOpcode, vData, wLength);
}

void ReadMemoryWindow(enc_t *chip,unsigned char vWindow, unsigned char *vData,int wLength)
{
	unsigned char vOpcode;
	vOpcode = RBMUDA;
	if(vWindow & GP_WINDOW)
		vOpcode = RBMGP;
	if(vWindow & RX_WINDOW)
		vOpcode = RBMRX;
	ReadN(chip,vOpcode, vData, wLength);
}
