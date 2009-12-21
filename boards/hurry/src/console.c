/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "console.h"

void console_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;
	
	/* Enable GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/*configure PA9<uart1.tx>, PA10<uart1.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = 115200;
	USART_Init(USART1, &uartinfo);
	USART_Cmd(USART1, ENABLE);
}

int console_putchar(char c)
{		
	USART_SendData(USART1, c);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(USART1, '\r');
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	}
	return 0;
}

int console_getchar(void)
{
	int ret;

	/*wait for a char*/
	while(1) {
		ret = console_IsNotEmpty();
		if(ret) break;
	}

	/*read and echo back*/
	ret = USART_ReceiveData(USART1);
	console_putchar((char)ret);
	return ret;
}

int console_IsNotEmpty()
{
	return (int) USART_GetFlagStatus(USART1, USART_FLAG_RXNE);
}