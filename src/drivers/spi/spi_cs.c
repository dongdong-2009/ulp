/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "spi.h"

#ifdef CONFIG_CPU_STM32
#include "stm32f10x.h"

void spi_cs_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	
#ifdef CONFIG_SPI_CS_PB1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

#ifdef CONFIG_SPI_CS_PB10
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

#ifdef CONFIG_SPI_CS_PC4
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif

#ifdef CONFIG_SPI_CS_PC5
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif

#ifdef CONFIG_SPI_CS_PF11
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
#endif
}

void spi_cs_set(int addr, int level)
{
	BitAction ba = Bit_RESET;
	if(level)
		ba = Bit_SET;
	
#ifdef CONFIG_SPI_CS_PB1
	if(addr == SPI_CS_PB1) {
		GPIO_WriteBit(GPIOB, GPIO_Pin_1, ba);
	}
#endif

#ifdef CONFIG_SPI_CS_PB10
	if(addr == SPI_CS_PB10) {
		GPIO_WriteBit(GPIOB, GPIO_Pin_10, ba);
	}
#endif

#ifdef CONFIG_SPI_CS_PC4
	if(addr == SPI_CS_PC4) {
		GPIO_WriteBit(GPIOC, GPIO_Pin_4, ba);
	}
#endif

#ifdef CONFIG_SPI_CS_PC5
	if(addr == SPI_CS_PC5) {
		GPIO_WriteBit(GPIOC, GPIO_Pin_5, ba);
	}
#endif

#ifdef CONFIG_SPI_CS_PF11
	if(addr == SPI_CS_PF11) {
		GPIO_WriteBit(GPIOF, GPIO_Pin_11, ba);
	}
#endif
}

#endif