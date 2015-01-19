/*
 *	junjun@2013 initial version
 *	miaofng@2013 fixed baudrate setting bug
 *	miaofng@2014-2-27 fixed swfifo bug & printf bypass console_xx (aduc putchar is called directly) issue
 *	- accroding to experiment, 115200 rx still has problem dueto poor arm7 irq performance
 *	- 57600 works very stable now
 */
#include "uart.h"
#include "aduc706x.h"
#include "common/circbuf.h"

#define RFSZ CONFIG_UART0_RF_SZ

#if RFSZ > 127
#error "fifo size is too big"
#endif

#if RFSZ > 1
static char uart_fifo[RFSZ];
static char uart_ridx, uart_widx;

void UART_IRQHandler(void)
{
	if(COMSTA0 & 1) {
		uart_fifo[uart_widx ++] = COMRX;
		uart_widx -= (uart_widx == RFSZ) ? RFSZ : 0;
	}
}
#endif

static int uart_Init(const uart_cfg_t *cfg)
{
	/*baudrate = (Fcore / 16 / 2) / DL / ( M + N / 2048)*/
	int M,N,DL,DELTA = SystemFrequency;
	int f = SystemFrequency / 16 / 2;
	for(char m = 1; m < 5; m ++) {
		int DL_min = f / cfg->baud / (m + 1) - 1;
		int DL_max = f / cfg->baud / m + 1;
		DL_min = (DL_min < 0) ? 0 : DL_min;
		DL_max = (DL_max > 0xffff) ? 0xffff : DL_max;
		for(int dl = DL_min; dl < DL_max; dl ++) {
			int N_min = f * 2048 / cfg->baud / dl - m * 2048 - 1;
			int N_max = f * 2048 / cfg->baud / dl - m * 2048 + 1;
			N_min = (N_min < 0) ? 0 : N_min;
			N_max = (N_max > 2047) ? 2047 : N_max;
			for(int n = N_min; n < N_max; n ++) {
				int bps = f * 2048 / dl / (m * 2048 + n);
				bps = bps - cfg->baud;
				bps = (bps < 0) ? (0 - bps) : bps;
				if(bps < DELTA) {
					M = m & 0x03; //4->0
					N = n;
					DL = dl;
					DELTA = bps;
				}
			}
		}
	}

	COMCON0 = 0x80; /*DLAB = 1*/
	COMDIV0 = DL & 0xff;
	COMDIV1 = DL >> 8;
	COMDIV2 = 0x8000 | (M << 11) | N;
	COMCON0 = 0x03; /*DLAB = 0, WLS = 11(8bits)*/
	GP1CON |= 0x11; // Select UART for P1.0/P1.1

#if RFSZ > 1
	uart_ridx = 0;
	uart_widx = 0;
	FIQEN |= IRQ_UART;
	COMIEN0 = 1; //Enable rx buf full irq
#endif
	return 0;
}

static int uart_putchar(int data)
{
	while(!(COMSTA0 & (1 << 6))); //wait until COMTX empty(TEMT = 1)
	COMTX = (data & 0xff);
	return 0;
}

static int uart_IsNotEmpty(void)
{
	int ret;
#if RFSZ > 1
	ret = uart_widx - uart_ridx;
	ret += (ret < 0) ? RFSZ : 0;
#else
	ret = COMSTA0 & 1;
#endif
	return ret;
}

static int uart_getch(void)
{
	int ret;
	while(!uart_IsNotEmpty());
#if RFSZ > 1
	ret = uart_fifo[uart_ridx ++];
	uart_ridx -= (uart_ridx == RFSZ) ? RFSZ : 0;
#else
	ret = COMRX;
#endif
	return ret;
}

uart_bus_t uart0 = {
	.init = uart_Init,
	.putchar = uart_putchar,
	.getchar = uart_getch,
	.poll = uart_IsNotEmpty,
#ifdef CONFIG_UART_KWP_SUPPORT
	.wake = NULL,
#endif
};
