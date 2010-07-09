/*
 * 	miaofng@2010 initial version
 *		This file is used to provide data link and phy level i/f for ISO14230/keyword2000 protocol
 *		send: DMA irq mode
 *		recv: DMA poll mode
 */

#include "config.h"
#include "kwp.h"
#include "time.h"
#include "stm32f10x.h"

#define __DEBUG
//#undef __DEBUG

static time_t kwp_timer;
static char kwp_stm;
static char *kwp_buf;
static char kwp_kbyte;

static void kwp_SetupTxDMA(void *p, size_t n);
static void kwp_SetupRxDMA(void *p, size_t n);

void kwp_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uartinfo;

	/* Enable GPIOA clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

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
#ifdef __DEBUG
	uartinfo.USART_BaudRate = 115200;
#else
	uartinfo.USART_BaudRate = KWP_BAUD;
#endif
	USART_Init(USART1, &uartinfo);
	USART_Cmd(USART1, ENABLE);

	kwp_stm = KWP_STM_IDLE;
	kwp_kbyte = 0xEA; //0 1 0 1,  0 1  1 1, force 4 byte header mode
}

int kwp_wake(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //uart1.tx
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOA, GPIO_Pin_9, BIT_RESET);
	kwp_timer = time_get(KWP_FAST_INIT_LOW_MS);
	kwp_stm = KWP_STM_WAKE_LO;
}

int kwp_isready(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	switch(kwp_stm) {
		case KWP_STM_WAKE_LO:
		if(time_left(kwp_timer) < 0) {
			GPIO_WriteBit(GPIOA, GPIO_Pin_9, BIT_SET);
			kwp_timer = KWP_STM_WAKE_HI;
			kwp_stm = KWP_STM_WAKE_HI;
		}
		break;
		case KWP_STM_WAKE_HI:
		if(time_left(kwp_timer) < 0) {
			GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
			GPIO_Init(GPIOA, &GPIO_InitStructure);
			kwp_stm = KWP_STM_START_COMM;
		}
		break;
		case KWP_STM_START_COMM:
		kwp_buf = kwp_malloc(1);
		kwp_buf[0] = SCR;
		kwp_trans(kwp_buf, 1);
		kwp_stm = KWP_STM_READ_KBYTES;
		break;
		case KWP_STM_READ_KBYTES:
		if(kwp_poll()) {
			kwp_kbyte = kwp_buf[0];
			kwp_free(kwp_buf);
			kwp_stm = KWP_STM_READY;
		}
		break;
		default:
		break;
	}

	return (kwp_stm == KWP_STM_READY);
}

/*format: sizeof_buf(4bytes) + kwp_head(4bytes?) + kwp_data(?) + cksum(1byte)*/
void *kwp_malloc(size_t n)
{
	int *p = malloc(sizeof(int) + sizeof(kwp_head_t) + 1 + n);
	*p = n;
	return (char *)p + sizeof(int) + sizeof(kwp_head_t);
}

void kwp_free(void *p)
{
	p = (char *)p - sizeof(kwp_head_t) - sizeof(int);
	free(p);
}

int kwp_transfer(char *buf, size_t n)
{
}

int kwp_poll(void)
{
}

static void kwp_SetupTxDMA(void *p, size_t n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA1_Channel4); //uart1 tx dma
	DMA_InitStructure.DMA_PeripheralBaseAddr = &USART1->DR;
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
}

static void kwp_SetupRxDMA(void *p, size_t n)
{
	DMA_InitTypeDef DMA_InitStructure;

	DMA_DeInit(DMA1_Channel5); //uart1 rx dma
	DMA_InitStructure.DMA_PeripheralBaseAddr = &USART1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)p;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
}
