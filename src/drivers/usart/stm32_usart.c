/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "stm32f10x.h"

int usart_IsNotEmpty(void);
int usart_getch(void);
int usart_putchar(char c);

void usart_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;

	/* Enable GPIOA clock */
#ifndef CONFIG_STM32_USART1
#ifdef STM32F10X_CL
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
#endif
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#else
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
#endif

#ifndef CONFIG_STM32_USART1
#ifdef STM32F10X_CL
	/*configure PD5<uart2.tx>, PD6<uart2.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
#else
	/*configure PA2<uart2.tx>, PA3<uart2.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
#else
	/*configure PA9<uart1.tx>, PA10<uart1.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = 115200;
#ifndef CONFIG_STM32_USART1
	USART_Init(USART2, &uartinfo);
	USART_Cmd(USART2, ENABLE);
#else
	USART_Init(USART1, &uartinfo);
	USART_Cmd(USART1, ENABLE);
#endif
}

int usart_putchar(char c)
{
#ifndef CONFIG_STM32_USART1
	USART_SendData(USART2, c);
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(USART2, '\r');
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	}
#else
	USART_SendData(USART1, c);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(USART1, '\r');
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	}
#endif
	return 0;
}

int usart_getch(void)
{
	int ret;

	/*wait for a char*/
	while(1) {
		ret = usart_IsNotEmpty();
		if(ret) break;
	}

	/*read and echo back*/
#ifndef CONFIG_STM32_USART1
	ret = USART_ReceiveData(USART2);
#else
	ret = USART_ReceiveData(USART1);
#endif
	return ret;
}

int usart_getchar(void)
{
	int ret = usart_getch();
	usart_putchar((char)ret);
	return ret;
}

int usart_IsNotEmpty()
{
#ifndef CONFIG_STM32_USART1
	return (int) USART_GetFlagStatus(USART2, USART_FLAG_RXNE);
#else
	return (int) USART_GetFlagStatus(USART1, USART_FLAG_RXNE);
#endif
}