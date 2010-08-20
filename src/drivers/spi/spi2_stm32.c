/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "spi.h"
#include "device.h"

#if CONFIG_DRIVER_SPI2 == 1
	#define spi SPI2
	#define dma_ch_tx DMA1_Channel5
	#define dma_ch_rx DMA1_Channel4
#endif

#if CONFIG_SPI2_TX_FIFO_SZ > 0
#define ENABLE_TX_DMA 1
#define TX_FIFO_SZ CONFIG_SPI2_TX_FIFO_SZ
#endif

#if CONFIG_SPI2_RX_FIFO_SZ > 0
#define ENABLE_RX_DMA 1
#define RX_FIFO_SZ CONFIG_SPI2_RX_FIFO_SZ
#endif

static int spi_Init(const spi_cfg_t *spi_cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	/* pin map:	SPI1		SPI2
		NSS		PA4		PB12
		SCK		PA5		PB13
		MISO	PA6		PB14
		MOSI	PA7		PB15
	*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*handle chip select pins*/
#ifdef CONFIG_SPI_CS_PB10
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = (spi_cfg->bits > 8) ? SPI_DataSize_16b : SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = (spi_cfg->cpol) ? SPI_CPOL_High : SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = (spi_cfg->cpha) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
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

	return 0;
}

static int spi_Write(int addr, int val)
{
	/*cs = low*/
#ifdef CONFIG_SPI_CS_PB10
	if(addr == SPI_CS_PB10) {
		GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_RESET);
	}
#endif

	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(spi, (uint16_t)val);
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET);

	/*cs = high*/
#ifdef CONFIG_SPI_CS_PB10
	if(addr == SPI_CS_PB10) {
		GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_SET);
	}
#endif

	return SPI_I2S_ReceiveData(spi);
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

spi_bus_t spi2 = {
	.init = spi_Init,
	.wreg = spi_Write,
	.rreg = NULL,
	.wbuf = spi_DMA_Write,
	.rbuf = NULL,
};
