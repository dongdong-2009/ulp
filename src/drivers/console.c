/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "console.h"

void console_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;
	
	/* Enable GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/*configure PA2<uart2.tx>, PA3<uart2.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = 115200;
	USART_Init(USART2, &uartinfo);
	USART_Cmd(USART2, ENABLE);
}

int console_putchar(char c)
{		
	USART_SendData(USART2, c);
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(USART2, '\r');
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
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
	ret = USART_ReceiveData(USART2);
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
	return (int) USART_GetFlagStatus(USART2, USART_FLAG_RXNE);
}