/*
 * 	miaofng@2010 initial version
 *		This file is used to provide phy level i/f for ISO14230/keyword2000 protocol
 *		send: DMA poll mode
 *		recv: DMA poll + cirbuf mode
 */

#include "config.h"
#include "kwd.h"
#include "time.h"
#include "stm32f10x.h"

/*note: rx fifo size must be big enough to hold all the data received during poll duration,
or overflow may occurs*/
#define KWD_RF_SZ 16
static char kwd_rf[KWD_RF_SZ]; /*rx fifo*/
static short kwd_rfn; /*total bytes came into rx fifo*/
static char *kwd_rbuf; /*buf to store the data received*/

static short kwd_tn;
static short kwd_rn;

static void kwd_SetupTxDMA(void *p, size_t n);
static void kwd_SetupRxDMA(void *p, size_t n);

void kwd_init(void)
{
	kwd_tn = 0;
	kwd_rn = 0;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;

	/* Enable GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/*configure PA9<uart1.tx>, PA10<uart1.rx>*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*init serial port*/
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = KWD_BAUD;
	USART_Init(USART1, &uartinfo);
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

int kwd_baud(int baud)
{
	USART_InitTypeDef uartinfo;
	USART_StructInit(&uartinfo);
	uartinfo.USART_BaudRate = baud;
	USART_Init(USART1, &uartinfo);
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	USART_Cmd(USART1, ENABLE);
	return 0;
}

void kwd_wake(int op)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	if(op == KWD_WKOP_LO) {
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //uart1.tx
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_RESET);
	}

	else if(op == KWD_WKOP_HI) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_SET);
	}
	
	else if(op == KWD_WKOP_RS) {
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
}

/* whole kwd_transfer procedure
1) disable uart
2) setup uart tx/rx dma
3) enable uart tx
4) ...dma isr()-> enable uart rx
5) ...poll() rx finish?
*/
int kwd_transfer(char *tbuf, size_t tn, char *rbuf, size_t rn)
{
	kwd_tn = (short)tn;
	kwd_rn = (short)rn;
	kwd_rbuf = rbuf;
	kwd_rfn = 0;

	//setup tx/rx phy engine
	kwd_SetupTxDMA(tbuf, tn);
	kwd_SetupRxDMA(kwd_rf, KWD_RF_SZ);
	return 0;
}

/*rx transfer: dma poll mode*/
int kwd_poll(int rx)
{
	int n;
	int n0, n1; /*index for fifo*/
	int i, fi;

	/*receive data from rx fifo?*/
	n0 = kwd_rfn % KWD_RF_SZ;
	n1 = KWD_RF_SZ - DMA_GetCurrDataCounter(DMA1_Channel5);
	
	n = n1 - n0;
	n += (n < 0) ? KWD_RF_SZ : 0;
	for(i = 0; i < n; i ++) {
		if(kwd_rfn > kwd_tn) { //store to rbuf
			fi = n0 + i; //Fifo Idx
			fi -= (fi >= KWD_RF_SZ) ? KWD_RF_SZ : 0;
			if(kwd_rfn - kwd_tn <= kwd_rn) { //rbuf not overflow
				*kwd_rbuf ++ = kwd_rf[n0 + i];
			}
		}
		else { //tx data echo back, ignore?
		}
		kwd_rfn ++;
	}
	
	/*return result*/
	if(rx && kwd_rn) {
		n = kwd_rfn - kwd_tn;
		n = (n > 0) ? n : 0;
	}
	
	if(!rx && kwd_tn) {
		n = kwd_rfn;
		n = (n > kwd_tn) ? kwd_tn : n;
	}

	return n;
}

static void kwd_SetupTxDMA(void *p, size_t n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_Cmd(DMA1_Channel4, DISABLE);
	DMA_DeInit(DMA1_Channel4); //uart1 tx dma
	DMA_InitStructure.DMA_PeripheralBaseAddr = (int)(&USART1->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)p;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel4, ENABLE);
}

static void kwd_SetupRxDMA(void *p, size_t n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_Cmd(DMA1_Channel5, DISABLE);
	DMA_DeInit(DMA1_Channel5); //uart1 rx dma
	DMA_InitStructure.DMA_PeripheralBaseAddr = (int)(&USART1->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)p;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel5, ENABLE);
}
