/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "spi.h"
#include "ulp/device.h"

#if CONFIG_DRIVER_SPI1 == 1
	#define spi SPI1
	#define dma_ch_tx DMA1_Channel3
	#define dma_ch_rx DMA1_Channel2
#endif

struct {
	unsigned char bits : 6;
	unsigned char csel : 1;
	unsigned char bseq : 1;
} spi_priv;

static int spi_Init(const spi_cfg_t *spi_cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	RCC_ClocksTypeDef RCC_ClocksStatus;
	int hz, prediv;

	spi_priv.csel = spi_cfg->csel;
	spi_priv.bits = spi_cfg->bits;
	spi_priv.bseq = spi_cfg->bseq;

	/* pin map:	SPI1		SPI2
		NSS		PA4		PB12
		SCK		PA5		PB13
		MISO	PA6		PB14
		MOSI	PA7		PB15
	*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
#if CONFIG_SPI1_MISO == 1
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_6;
#endif
#ifdef CONFIG_SPI1_CS_HARD
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_4;
#endif
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*clock setting*/
	RCC_GetClocksFreq(&RCC_ClocksStatus);
	prediv = 1; //default setting: Fapb2(72Mhz)/4
	if(spi_cfg -> freq != 0) {
		hz = RCC_ClocksStatus.PCLK2_Frequency >> 1;
		for( prediv = 0; hz > spi_cfg -> freq; prediv ++ ) {
			hz >>= 1;
		}

		if(prediv > 7)
			prediv = 7;
	}

	/* SPI configuration */
	SPI_Cmd(spi, DISABLE);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = (spi_cfg->bits == 16) ? SPI_DataSize_16b : SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = (spi_cfg->cpol) ? SPI_CPOL_High : SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = (spi_cfg->cpha) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
#ifdef CONFIG_SPI1_CS_HARD
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
#else
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
#endif
	SPI_InitStructure.SPI_BaudRatePrescaler = prediv << 3;
	SPI_InitStructure.SPI_FirstBit = (spi_cfg->bseq) ? SPI_FirstBit_MSB : SPI_FirstBit_LSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &SPI_InitStructure);
#ifdef CONFIG_SPI1_CS_HARD
	SPI_SSOutputCmd(spi, ENABLE);
#endif

	spi_cs_init();

#ifdef CONFIG_SPI1_DMA
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	SPI_I2S_DMACmd(spi, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_I2S_DMACmd(spi, SPI_I2S_DMAReq_Rx, ENABLE);
	DMA_Cmd(dma_ch_tx, DISABLE);
	DMA_Cmd(dma_ch_rx, DISABLE);
#endif

	/* Enable the SPI  */
	SPI_Cmd(spi, ENABLE);
	return 0;
}

static int __spi_write(int val)
{
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(spi, (uint16_t)val);
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(spi);
}

#ifdef CONFIG_SPI1_CS_SOFT
static inline int __spi_csel(int addr, int level)
{
	if(!spi_priv.csel)
		return spi_cs_set(addr, level);
	else
		return 0;
}
#else
#define __spi_csel(addr, level)
#endif

static int spi_Write(int addr, int val)
{
	int dw, dr, ret, i, dn, n = spi_priv.bits;
	dn = (n == 16) ? 16 : 8;

	__spi_csel(addr, 0);
	for(ret = 0, i = 0; i < n; i += dn) {
		if(spi_priv.bseq) { //msb, high bytes first
			dw = val >> (n - i - dn);
			dw &= (1 << dn) - 1;
			dr = __spi_write(dw);
			ret <<= dn;
			ret |= dr;
		}
		else {
			dw = val >> i;
			dr = __spi_write(dw);
			ret |= dr << i;
		}
	}
	__spi_csel(addr, 1);
	return ret;
}

static int spi_Read(int addr)
{
	return spi_Write(addr, CONFIG_SPI_READ_DUMMY);
}

#ifdef CONFIG_SPI1_DMA
static int spi_wbuf(const char *wbuf, char *rbuf, int n)
{
	DMA_InitTypeDef  DMA_InitStructure;
	DMA_Cmd(dma_ch_tx, DISABLE);
	DMA_Cmd(dma_ch_rx, DISABLE);

	//setup tx dma
	DMA_DeInit(dma_ch_tx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&spi->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) wbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(dma_ch_tx, &DMA_InitStructure);

	//setup rx dma
	DMA_DeInit(dma_ch_rx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&spi->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) rbuf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(dma_ch_rx, &DMA_InitStructure);

	DMA_Cmd(dma_ch_tx, ENABLE);
	DMA_Cmd(dma_ch_rx, ENABLE);
	return 0;
}

static int spi_poll(void)
{
	return DMA_GetCurrDataCounter(dma_ch_rx);
}
#endif

const spi_bus_t spi1 = {
	.init = spi_Init,
	.wreg = spi_Write,
	.rreg = spi_Read,
#ifndef CONFIG_SPI1_CS_SOFT
	.csel = NULL,
#else
	.csel = spi_cs_set,
#endif

#ifdef CONFIG_SPI1_DMA
	.wbuf = spi_wbuf,
	.poll = spi_poll,
#else
	.wbuf = NULL,
	.poll = NULL,
#endif
};
