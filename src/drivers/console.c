/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "console.h"
#include "uart.h"
#include <stddef.h>
#include "sys/sys.h"
#include "debug.h"

static const struct console_s *cnsl; /*current console*/

void console_Init(void)
{
	uart_cfg_t cfg = UART_CFG_DEF;
	cfg.baud = BAUD_115200;
#ifdef CONFIG_CONSOLE_BAUD
	cfg.baud = CONFIG_CONSOLE_BAUD;
#endif

#ifdef CONFIG_CONSOLE_UARTg
	uartg.init(&cfg);
	cnsl = (const struct console_s *) &uartg;
#endif
#ifdef CONFIG_CONSOLE_UART0
	uart0.init(&cfg);
	cnsl = (const struct console_s *) &uart0;
#endif
#ifdef CONFIG_CONSOLE_UART1
	uart1.init(&cfg);
	cnsl = (const struct console_s *) &uart1;
#endif
#ifdef CONFIG_CONSOLE_UART2
	uart2.init(&cfg);
	cnsl = (const struct console_s *) &uart2;
#endif
}

int console_select(const struct console_s *new)
{
	cnsl = new;
	return 0;
}

int console_putch(char c)
{
	assert(cnsl != NULL);
	cnsl -> putchar(c);
	return 0;
}

int console_getch(void)
{
	assert(cnsl != NULL);
	return cnsl -> getchar();
}

int console_putchar(char c)
{
	console_putch(c);
	if(c == '\n') {
		console_putch('\r');
	}
	return 0;
}

int console_getchar(void)
{
	int ret = console_getch();
	console_putch((char) ret);
	return ret;
}

int console_IsNotEmpty(void)
{
	assert(cnsl != NULL);
	return cnsl -> poll();
}
