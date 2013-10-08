/*
 *	junjun@2013 initial version
 *	miaofng@2013 fixed baudrate setting bug
 */
#include "uart.h"
#include "aduc706x.h"
#include "common/circbuf.h"

#if CONFIG_UART0_RF_SZ > 0
static circbuf_t uart_fifo_rx;

void UART_IRQHandler(void)
{
	if(UrtSta()&1) {
		int c = getchar();
		buf_push(&uart_fifo_rx, &c, 1);
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

#if CONFIG_UART0_RF_SZ > 0
	buf_init(&uart_fifo_rx, CONFIG_UART0_RF_SZ);
	IRQEN |= IRQ_UART;
	COMIEN0 |= 1; //Enable rx buf full irq
#endif
	return 0;
}

static int uart_putchar(int data)
{
	putchar(data & 0xff);
	return 0;
}

static int uart_IsNotEmpty(void)
{
	int ret;
#if CONFIG_UART0_RF_SZ > 0
	ret = buf_size(&uart_fifo_rx);
#else
	ret = UrtSta()&1;
#endif
	return ret;
}

static int uart_getch(void)
{
	int ret;
	while(!uart_IsNotEmpty());
#if CONFIG_UART0_RF_SZ > 0
	ret = 0;
	buf_pop(&uart_fifo_rx, &ret, 1);
#else
	ret = getchar();
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
