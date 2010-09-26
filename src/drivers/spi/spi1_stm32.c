/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "spi.h"
#include "device.h"

#if CONFIG_DRIVER_SPI1 == 1
	#define spi SPI1
	#define dma_ch_tx DMA1_Channel3
	#define dma_ch_rx DMA1_Channel2
#endif

#if CONFIG_SPI1_TX_FIFO_SZ > 0
#define ENABLE_TX_DMA 1
#define TX_FIFO_SZ CONFIG_SPI1_TX_FIFO_SZ
#endif

#if CONFIG_SPI1_RX_FIFO_SZ > 0
#define ENABLE_RX_DMA 1
#define RX_FIFO_SZ CONFIG_SPI1_RX_FIFO_SZ
#endif

#ifdef CONFIG_SPI1_CS_SOFT
static char flag_csel;
#endif

static int spi_Init(const spi_cfg_t *spi_cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
#ifdef CONFIG_SPI1_CS_SOFT
	flag_csel = spi_cfg -> csel;
#endif
	
	/* pin map:	SPI1		SPI2
		NSS		PA4		PB12
		SCK		PA5		PB13
		MISO	PA6		PB14
		MOSI	PA7		PB15
	*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
#ifdef CONFIG_SPI1_CS_HARD
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_4;
#endif
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = (spi_cfg->bits > 8) ? SPI_DataSize_16b : SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = (spi_cfg->cpol) ? SPI_CPOL_High : SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = (spi_cfg->cpha) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
#ifdef CONFIG_SPI1_CS_HARD
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
#else
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
#endif
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = (spi_cfg->bseq) ? SPI_FirstBit_MSB : SPI_FirstBit_LSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &SPI_InitStructure);
	
	/* Enable the SPI  */
	SPI_Cmd(spi, ENABLE);

#ifdef ENABLE_TX_DMA
	DMA_InitTypeDef  DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(dma_ch_tx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&spi->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = 0;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(dma_ch_tx, &DMA_InitStructure);

	SPI_I2S_DMACmd(spi, SPI_I2S_DMAReq_Tx, ENABLE);
#endif

#ifdef CONFIG_SPI1_CS_SOFT
	spi_cs_init();
#endif
	return 0;
}

static int spi_Write(int addr, int val)
{
	int ret = 0;

#ifdef CONFIG_SPI1_CS_SOFT
	/*cs low*/
	if(!flag_csel)
		spi_cs_set(addr, 0);
#endif
	
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(spi, (uint16_t)val);
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET);
	ret = SPI_I2S_ReceiveData(spi);

#ifdef CONFIG_SPI1_CS_SOFT
	/*cs high*/
	if(!flag_csel)
		spi_cs_set(addr, 1);
#endif

	return ret;
}

static int spi_Read(int addr)
{
	return spi_Write(addr, 0xff);
}

static int spi_DMA_Write(char *pbuf, int len)
{
#ifdef ENABLE_TX_DMA
	if (DMA_GetCurrDataCounter(dma_ch_tx))
		return 1;
	DMA_Cmd(dma_ch_tx, DISABLE);
	dma_ch_tx->CMAR = (uint32_t)pbuf;
	dma_ch_tx->CNDTR = len;
	DMA_Cmd(dma_ch_tx, ENABLE);
#endif
	return 0;
}

spi_bus_t spi1 = {
	.init = spi_Init,
	.wreg = spi_Write,
	.rreg = spi_Read,
#ifndef CONFIG_SPI1_CS_SOFT
	.csel = NULL,
#else
	.csel = spi_cs_set,
#endif
	
	/*reserved*/
	.wbuf = spi_DMA_Write,
	.rbuf = NULL,
};
