/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "console.h"
#include "uart.h"

uart_bus_t *uart_con;

void console_Init(void)
{
	uart_cfg_t cfg = UART_CFG_DEF;

#ifdef CONFIG_CONSOLE_UART1
	uart_con = &uart1;
#endif
#ifdef CONFIG_CONSOLE_UART2
	uart_con = &uart2;
#endif

	cfg.baud = BAUD_115200;
	uart_con -> init(&cfg);
}

int console_putchar(char c)
{
	uart_con -> putchar(c);
	if(c == '\n') {
		uart_con -> putchar('\r');
	}
	return 0;
}

int console_getch(void)
{
	int ret = uart_con -> getchar();
	return ret;
}

int console_getchar(void)
{
	int ret = console_getch();
	console_putchar((char) ret);
	return ret;
}

int console_IsNotEmpty(void)
{
	return uart_con -> poll();
}
