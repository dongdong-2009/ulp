/*
*	miaofng@2010 initial version
*/

#include "stm32f10x.h"
#include "spi.h"

#define bus_to_spi(bus)	(bus == 1) ? SPI1 : ((bus == 2)? SPI2 : 0)

#define SPI1_DR_Base		0x4001300C
#define SPI2_DR_Base		0x4000380C

int spi_Init(int bus, int mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	SPI_TypeDef* spix = bus_to_spi(bus);
	if(!spix)
		return -1;

	/* pin map:	SPI1		SPI2
		NSS		PA4		PB12
		SCK		PA5		PB13
		MISO	PA6		PB14
		MOSI	PA7		PB15
	*/
	if(bus == 1) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;//|GPIO_Pin_4;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}

	if(bus == 2) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;//|GPIO_Pin_12;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = (mode & SPI_MODE_BW_16) ? SPI_DataSize_16b : SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = (mode & SPI_MODE_POL_1) ? SPI_CPOL_High : SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = (mode & SPI_MODE_PHA_1) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = (mode & SPI_MODE_PRESCALER_4) ? SPI_BaudRatePrescaler_4 : SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = (mode & SPI_MODE_LSB) ? SPI_FirstBit_LSB : SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(spix, &SPI_InitStructure);
	/* Enable the SPI  */
	SPI_Cmd(spix, ENABLE);

#ifdef CONFIG_DRIVER_SPI_STM32_DMA
	DMA_InitTypeDef  DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	if(bus == 1){
		/* SPI1 TX DMA configuration */
		DMA_DeInit(DMA1_Channel3);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SPI1_DR_Base;
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
		DMA_Init(DMA1_Channel3, &DMA_InitStructure);

		/* Enable SPI1 DMA Tx request */
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	}

	if (bus == 2) {
		/* SPI2 TX DMA configuration */
		DMA_DeInit(DMA1_Channel5);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SPI2_DR_Base;
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_BufferSize = 0;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init(DMA1_Channel5, &DMA_InitStructure);
		
		/* Enable SPI2 DMA Tx request */
		SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	}
#endif

	return 0;
}

int spi_Write(int bus, int val)
{
	SPI_TypeDef* spix = bus_to_spi(bus);
	if(!spix)
		return -1;

	while (SPI_I2S_GetFlagStatus(spix, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(spix, (uint16_t)val);
	while (SPI_I2S_GetFlagStatus(spix, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(spix);
}

/*
 *
 *len : number of Bytes to be transfered
 */
void spi_DMA_Write(int bus, unsigned char *pbuf, int len)
{
#ifdef CONFIG_DRIVER_SPI_STM32_DMA
	if (bus == 1) {
		while(DMA_GetCurrDataCounter(DMA1_Channel3));
		DMA_Cmd(DMA1_Channel3, DISABLE);
		DMA1_Channel3->CMAR = (uint32_t)pbuf;
		DMA1_Channel3->CNDTR = len;
		DMA_Cmd(DMA1_Channel3, ENABLE);
	}

	if (bus == 2) {
		while(DMA_GetCurrDataCounter(DMA1_Channel5));
		DMA_Cmd(DMA1_Channel5, DISABLE);
		DMA1_Channel5->CMAR = (uint32_t)pbuf;
		DMA1_Channel5->CNDTR = len;
		DMA_Cmd(DMA1_Channel5, ENABLE);	
	}
#endif
}
