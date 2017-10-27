/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "console.h"
#include "uart.h"
#include <stddef.h>
#include "sys/sys.h"
#include "ulp/debug.h"
#include "ulp/device.h"

static const struct console_s *cnsl; /*current console*/

#ifdef CONFIG_SHELL_MULTI
static const struct console_s *cnsl_def; /*system default console*/
static const struct console_s *cnsl_old;
#endif

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
#ifdef CONFIG_CONSOLE_UART3
	uart3.init(&cfg);
	cnsl = (const struct console_s *) &uart2;
#endif
#ifdef CONFIG_CONSOLE_UART4
	uart4.init(&cfg);
	cnsl = (const struct console_s *) &uart4;
#endif
#ifdef CONFIG_SHELL_MULTI
	cnsl_def = cnsl;
#endif
}

int console_set(const struct console_s *new)
{
#ifdef CONFIG_SHELL_MULTI
	cnsl_old = cnsl;
	cnsl = (new == NULL) ? cnsl_def : new;
#endif
	return 0;
}

const struct console_s* console_get(void)
{
	return cnsl;
}

int console_putch(char c)
{
	//assert(cnsl != NULL);
	if(cnsl->init != NULL)
		cnsl -> putchar(c);
	else {
		dev_write(cnsl->fd, &c, 1);
	}
	return 0;
}

int console_getch(void)
{
	char c;
	if(cnsl->init != NULL)
		c = cnsl -> getchar();
	else {
		dev_read(cnsl->fd, &c, 1);
	}
	return c;
}

void console_flush(void)
{
	while(console_IsNotEmpty()) {
		console_getch();
	}
}

int console_putchar(char c)
{
	if(c == '\n') {
		console_putch('\r');
	}
	console_putch(c);
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
	int n = 0;
	if(cnsl->init != NULL)
		n = cnsl -> poll();
	else {
		n = dev_poll(cnsl->fd, POLLIN);
	}
	return n;
}

#ifdef CONFIG_SHELL_MULTI
int console_restore(void)
{
	const struct console_s *tmp;
	tmp = cnsl;
	cnsl = cnsl_old;
	cnsl_old = tmp;
	return 0;
}

/*patch for new style device driver*/
struct console_s * console_register(int fd)
{
	struct console_s *cnsl = sys_malloc(sizeof(struct console_s));
	if(cnsl != NULL) {
		cnsl->init = NULL;
		cnsl->putchar = NULL;
		cnsl->getchar = NULL;
		cnsl->poll = NULL;
		cnsl->fd = fd;
	}
	return cnsl;
}

int console_unregister(struct console_s *cnsl)
{
	sys_free(cnsl);
	return 0;
}
#endif
