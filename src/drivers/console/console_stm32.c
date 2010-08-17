/* console.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "console.h"

#if CONFIG_CONSOLE_UART1 == 1
	#define uart USART1
	#define dma_ch_tx DMA1_Channel4
	#define dma_ch_rx DMA1_Channel5
#elif CONFIG_CONSOLE_UART2 == 1
	#define uart USART2
	#define dma_ch_tx DMA1_Channel7
	#define dma_ch_rx DMA1_Channel6
#else 
	//default to uart2
	#define CONFIG_CONSOLE_UART2 1
	#define uart USART2
	#define dma_ch_tx DMA1_Channel7
	#define dma_ch_rx DMA1_Channel6
#endif

//debug
#define CONFIG_CONSOLE_TX_FIFO_SZ 64
#define CONFIG_CONSOLE_RX_FIFO_SZ 16

#if CONFIG_CONSOLE_TX_FIFO_SZ > 0
#define ENABLE_TX_DMA 1
#define TX_FIFO_SZ CONFIG_CONSOLE_TX_FIFO_SZ
static char console_fifo_tx[TX_FIFO_SZ];
static char console_fifo_tn;
static void console_SetupTxDMA(void *p, int n);
#endif

#if CONFIG_CONSOLE_RX_FIFO_SZ > 0
#define ENABLE_RX_DMA 1
#define RX_FIFO_SZ CONFIG_CONSOLE_RX_FIFO_SZ
static char console_fifo_rx[CONFIG_CONSOLE_RX_FIFO_SZ];
static char console_fifo_rn;
static void console_SetupRxDMA(void *p, int n);
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

#ifdef ENABLE_TX_DMA
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	console_SetupTxDMA(console_fifo_tx, 0);
	USART_DMACmd(uart, USART_DMAReq_Tx, ENABLE);
	console_fifo_tn = 0;
#endif

#ifdef ENABLE_RX_DMA
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	console_SetupRxDMA(console_fifo_rx, RX_FIFO_SZ);
	USART_DMACmd(uart, USART_DMAReq_Rx, ENABLE);
	console_fifo_rn = 0;
#endif

	USART_Cmd(uart, ENABLE);
}

int console_putchar(char c)
{
#ifdef ENABLE_TX_DMA
	int i, j, n;
	
	//wait ... no enough fifo space to use
	do {
		n = DMA_GetCurrDataCounter(dma_ch_tx);
	} while(TX_FIFO_SZ - n < 2);
	
	//copy
	DMA_Cmd(dma_ch_tx, DISABLE);
	n = DMA_GetCurrDataCounter(dma_ch_tx);
	for(i = 0, j = console_fifo_tn - n; i < n; i ++, j ++) {
		if(i != j)
			console_fifo_tx[i] = console_fifo_tx[j];
	}
	
	console_fifo_tx[i ++] = c;
	if(c == '\n') {
		console_fifo_tx[i ++] = '\r';
	}
	
	console_fifo_tn = i;
	console_SetupTxDMA(console_fifo_tx, i);
#else
	USART_SendData(uart, c);
	while(USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET);
	if(c == '\n') {
		USART_SendData(uart, '\r');
		while(USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET);
	}
#endif
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

#ifdef ENABLE_RX_DMA
	ret = console_fifo_rx[console_fifo_rn];
	console_fifo_rn ++;
	if(console_fifo_rn >= RX_FIFO_SZ)
		console_fifo_rn = 0;
#else
	ret = USART_ReceiveData(uart);
#endif
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
	int ret;
#ifdef ENABLE_RX_DMA
	int rn = RX_FIFO_SZ - DMA_GetCurrDataCounter(dma_ch_rx);
	ret = rn - console_fifo_rn;
	ret += (ret < 0) ? RX_FIFO_SZ : 0;
#else
	ret = (int) USART_GetFlagStatus(uart, USART_FLAG_RXNE);
#endif
	return ret;
}

#ifdef ENABLE_TX_DMA
static void console_SetupTxDMA(void *p, int n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_Cmd(dma_ch_tx, DISABLE);
	DMA_DeInit(dma_ch_tx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (int)(&uart->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)p;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(dma_ch_tx, &DMA_InitStructure);
	DMA_Cmd(dma_ch_tx, ENABLE);
}
#endif

#ifdef ENABLE_RX_DMA
static void console_SetupRxDMA(void *p, int n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_Cmd(dma_ch_rx, DISABLE);
	DMA_DeInit(dma_ch_rx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (int)(&uart->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)p;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(dma_ch_rx, &DMA_InitStructure);
	DMA_Cmd(dma_ch_rx, ENABLE);
}
#endif
