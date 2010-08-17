/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "console.h"

#if CONFIG_CONSOLE_UART1 == 1
	#define uart USART1
#elif CONFIG_CONSOLE_UART2 == 1
	#define uart USART2
#else 
	//default to uart2
	#define CONFIG_CONSOLE_UART2 1
	#define uart USART2
#endif

void console_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;

#ifdef CONFIG_CONSOLE_UART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#ifdef CONFIG_STM32F10X_CL
	/*configure PD5<uart2.tx>, PD6<uart2.rx>*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif
#endif /*CONFIG_CONSOLE_UART2*/

#if CONFIG_CONSOLE_UART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	/*configure PA9<uart1.tx>, PA10<uart1.rx>*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif /*CONFIG_CONSOLE_UART1*/

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = 115200;
	USART_Init(uart, &uartinfo);
	USART_Cmd(uart, ENABLE);
}

int console_putchar(char c)
{
	USART_SendData(uart, c);
	while(USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(uart, '\r');
		while(USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET);
	}
	return 0;
}

int console_getch(void)
{
	int ret;

	/*wait for a char*/
	while(1) {
		ret = console_IsNotEmpty();
		if(ret) break;
	}

	/*read and echo back*/
	ret = USART_ReceiveData(uart);
	return ret;
}

int console_getchar(void)
{
	int ret = console_getch();
	console_putchar((char)ret);
	return ret;
}

int console_IsNotEmpty()
{
	return (int) USART_GetFlagStatus(uart, USART_FLAG_RXNE);
}
