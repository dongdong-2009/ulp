/*
 *	junjun@2013 initial version
 */
#include "uart.h"
#include "aduc706x.h"


#define BIT15 0x8000
#define BIT11 0x800

static int uart_Init(const uart_cfg_t *cfg)
{
	//Initialize the UART for115200-8-N-1
	unsigned baud,i=0;
	GP1CON = 0x11; // Select UART for P1.0/P1.1
	baud = cfg->baud/600;
	if(baud<64)
		while(baud>>++i);
	else{
		baud /= 96;
		while(baud>>++i);
		i += 7;
	}
	UrtCfg(0,i-1,URT_68|URT_78);
	i = (SystemFrequency<<6)/cfg->baud;
	i %= 2048;
	COMDIV2 = i + 0x8800;
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
	ret = UrtSta()&1;
	return ret;
}

static int uart_getch(void)
{
	int ret;
	while(!uart_IsNotEmpty());
	ret = getchar();
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
