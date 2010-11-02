/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "spi.h"
#include "device.h"

#if CONFIG_DRIVER_SPI3 == 1
	#define spi SPI3
	#define dma_ch_tx DMA2_Channel2
	#define dma_ch_rx DMA2_Channel1
#endif

#ifdef CONFIG_SPI3_CS_SOFT
static char flag_csel;
#endif

static int spi_Init(const spi_cfg_t *spi_cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	RCC_ClocksTypeDef RCC_ClocksStatus;
	int hz, prediv;

#ifdef CONFIG_SPI3_CS_SOFT
	flag_csel = spi_cfg -> csel;
#endif

	/* pin map:		def		remap
		NSS		PA15
		SCK		PB3		PC10
		MISO	PB4		PC11
		MOSI	PB5		PC12
	*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#ifdef CONFIG_SPI3_REMAP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#else
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

#ifdef CONFIG_SPI3_CS_HARD
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_15;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	/*clock setting*/
	RCC_GetClocksFreq(&RCC_ClocksStatus);
	prediv = 0; //default setting: Fapb1(36Mhz)/2
	if(spi_cfg -> freq != 0) {
		hz = RCC_ClocksStatus.PCLK1_Frequency >> 1;
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
	SPI_InitStructure.SPI_DataSize = (spi_cfg->bits > 8) ? SPI_DataSize_16b : SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = (spi_cfg->cpol) ? SPI_CPOL_High : SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = (spi_cfg->cpha) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
#ifdef CONFIG_SPI3_CS_HARD
	SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
#else
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
#endif
	SPI_InitStructure.SPI_BaudRatePrescaler = prediv << 3;
	SPI_InitStructure.SPI_FirstBit = (spi_cfg->bseq) ? SPI_FirstBit_MSB : SPI_FirstBit_LSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spi, &SPI_InitStructure);
#ifdef CONFIG_SPI3_CS_HARD
	SPI_SSOutputCmd(spi, ENABLE);
#endif

#ifdef CONFIG_SPI3_CS_SOFT
	spi_cs_init();
#endif

#ifdef CONFIG_SPI3_DMA
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

static int spi_Write(int addr, int val)
{
	int ret = 0;

#ifdef CONFIG_SPI3_CS_SOFT
	/*cs low*/
	if(!flag_csel)
		spi_cs_set(addr, 0);
#endif

	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(spi, (uint16_t)val);
	while (SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET);
	ret = SPI_I2S_ReceiveData(spi);

#ifdef CONFIG_SPI3_CS_SOFT
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

#ifdef CONFIG_SPI3_DMA
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

const spi_bus_t spi3 = {
	.init = spi_Init,
	.wreg = spi_Write,
	.rreg = spi_Read,
#ifndef CONFIG_SPI3_CS_SOFT
	.csel = NULL,
#else
	.csel = spi_cs_set,
#endif

#ifdef CONFIG_SPI3_DMA
	.wbuf = spi_wbuf,
	.poll = spi_poll,
#else
	.wbuf = NULL,
	.poll = NULL,
#endif
};
