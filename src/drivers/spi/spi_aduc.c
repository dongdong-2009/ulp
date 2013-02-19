/*
*	miaofng@2013-1-4 initial version
*/

#include "config.h"
#include "aduc706x.h"
#include "spi.h"

//#define CONFIG_SPI_DEBUG

typedef union {
	struct spi_config_s {
		unsigned SPIEN : 1; /*spi enable*/
		unsigned SPIMEN : 1; /*master mode enable*/
		unsigned SPICPH : 1; /*clock phase, clock pulses at the begining(1)/end(0) of each serial bit transfer*/
		unsigned SPICPO : 1; /*clock polarity, 0->idle low*/

		unsigned SPIWOM : 1; /*1->wired or mode (open drain mode)*/
		unsigned SPILF : 1; /*1->LSB first*/
		unsigned SPITMDE : 1; /*Tx Mode Enable, 1->TxIRQ is set*/
		unsigned SPIZEN : 1; /*zero enable when txfifo empty, 1->zero, 0->last value*/

		unsigned SPIROW : 1; /*rx overflow enable*/
		unsigned SPIOEN : 1; /*miSO slave mode output enable, 0->open drain*/
		unsigned SPILP : 1; /*loop back*/
		unsigned SPICONT : 1; /*1->continuous transfer mode, 0->one stall cycle is inserted*/

		unsigned SPIRFLH : 1; /*rx flush*/
		unsigned SPITFLH : 1; /*tx flush*/
		unsigned SPIMDE : 2; /*irq mode, 00->1byte, 01->1byte, 10->2byte, 11->3byte*/
	};
	unsigned value;
} spi_config_t;

typedef union {
	struct spi_status_s {
		unsigned SPIISTA : 1; /*irq status*/
		unsigned SPITXFSTA : 3; /*txfifo bytes, 0-4 bytes*/

		unsigned SPITXUF : 1; /*txfifo underflow*/
		unsigned SPITXIRQ : 1; /*tx irq*/
		unsigned SPIRXIRQ : 1; /*rx irq*/
		unsigned SPIFOF : 1; /*rxfifo overflow*/

		unsigned SPIRXFSTA : 3; /*rxfifo bytes*/
		unsigned SPIREX : 1; /*there are more bytes in rxfifo than SPIMDE*/
	};
	unsigned value;
} spi_status_t;

#ifdef CONFIG_SPI_CS_SOFT
static struct {
	char csel : 1;
} spi_flag;
#endif

#ifdef CONFIG_SPI_VCHIP
#include "common/vchip.h"
static vchip_t vchip;

#ifdef CONFIG_SPI_DEBUG
#include "common/circbuf.h"
circbuf_t fifo;
#endif

void SPI_IRQHandler(void)
{
	char byte;
	spi_status_t status;

	status.value = SPISTA;
	if(status.SPIRXIRQ) {
		while(status.SPIRXFSTA > 0) {
			byte = SPIRX;
#ifdef CONFIG_SPI_DEBUG
			buf_push(&fifo, &byte, 1);
#endif
			vchip.rxd = &byte;
			vchip_update(&vchip);
			if(0){//(vchip.flag_soh == 1) && (vchip.flag_cmd == 0)) {
				//FLUSH TX
				SPICON |= (1 << 13);
				SPICON &= ~(1 << 13);
			}
			if(vchip.txd != NULL) {
				byte = *vchip.txd;
				SPITX = byte;
				vchip.txd = NULL;
			}
			status.value = SPISTA;
		}
	}
}
#endif

static int spi_Init(const spi_cfg_t *spi_cfg)
{
	SPICON = 0; //disable 1st to avoid garbage
#ifdef CONFIG_SPI_CS_SOFT
	spi_flag.csel = spi_cfg -> csel;
#endif

	/* pin map:		def
		NSS		P00
		SCK		P01
		MISO		P02
		MOSI		P03
	*/
	unsigned mask = 0X0000FFFF;
	unsigned value = 0X00001111;

#ifdef CONFIG_SPI_CS_NONE
	mask = 0X0000FFF0; //NSS = ANY VALUE
#endif

	GP0CON0 &= ~ mask;
	GP0CON0 |= (value & mask);

	/*SPI or ADC or IIC?*/
	GP0KEY1 = 0X7;
	GP0CON1 = 0X00;
	GP0KEY2 = 0X13;

#ifdef CONFIG_SPI_CS_SOFT
	spi_cs_init();
#endif
	spi_config_t spi_config = {
		.SPIEN = 1,
		#ifdef CONFIG_SPI_MASTER
		.SPIMEN = 1,
		#else
		.SPIMEN = 0,
		#endif
		.SPICPH = 0,
		.SPICPO = 0,

		.SPIWOM = 0,
		.SPILF = 0,
		.SPITMDE = 0, /*always in receive mode!!!*/
		.SPIZEN = 1,

		.SPIROW = 1,
		.SPIOEN = 1,
		.SPILP = 0,
		.SPICONT = 1,

		.SPIRFLH = 0,
		.SPITFLH = 0,
		.SPIMDE = 0,
	};

	spi_config.SPICPH = spi_cfg->cpha;
	spi_config.SPICPO = spi_cfg->cpol;
	spi_config.SPILF = !spi_cfg->bseq;
	SPICON = spi_config.value;

	int hz, div = 0;
	if(spi_cfg -> freq != 0) {
		do {
			hz = SystemFrequency_UCLK / 2 / (1 + div);
			div ++;
		} while((hz > spi_cfg -> freq) && (div < 256));
                SPIDIV = (char) div;
	}
#ifdef CONFIG_SPI_VCHIP
	#ifdef CONFIG_SPI_DEBUG
	buf_init(&fifo, 32);
	#endif
	vchip_reset(&vchip);
	IRQEN |= IRQ_SPI;
#endif
	return 0;
}

static int spi_Write(int addr, int val)
{
	int ret = 0;

#ifdef CONFIG_SPI_CS_SOFT
	/*cs low*/
	if(!spi_flag.csel)
		spi_cs_set(addr, 0);
#endif

	spi_status_t status;
	SPITX = val;
	do {
		status.value = SPISTA;
	} while(status.SPITXFSTA != 0);

#ifdef CONFIG_SPI_CS_SOFT
	/*cs high*/
	if(!spi_flag.csel)
		spi_cs_set(addr, 1);
#endif

	return ret;
}

static int spi_Read(int addr)
{
#ifdef CONFIG_SPI_MASTER
	spi_Write(addr, 0xff);
#endif
	return SPIRX;
}

const spi_bus_t spi = {
	.init = spi_Init,
	.wreg = spi_Write,
	.rreg = spi_Read,
};
