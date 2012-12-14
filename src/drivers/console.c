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
static const struct console_s *cnsl_def; /*system default console*/
static const struct console_s *cnsl_old;

void console_Init(void)
{
	uart_cfg_t cfg = UART_CFG_DEF;
	cfg.baud = BAUD_115200;
#ifdef CONFIG_CONSOLE_BAUD
	cfg.baud = CONFIG_CONSOLE_BAUD;
#endif

#ifdef CONFIG_CONSOLE_UARTg
	uartg.init(&cfg);
	cnsl_def = cnsl = (const struct console_s *) &uartg;
#endif
#ifdef CONFIG_CONSOLE_UART0
	uart0.init(&cfg);
	cnsl_def = cnsl = (const struct console_s *) &uart0;
#endif
#ifdef CONFIG_CONSOLE_UART1
	uart1.init(&cfg);
	cnsl_def = cnsl = (const struct console_s *) &uart1;
#endif
#ifdef CONFIG_CONSOLE_UART2
	uart2.init(&cfg);
	cnsl_def = cnsl = (const struct console_s *) &uart2;
#endif
}

const struct console_s* console_get(void)
{
	return cnsl;
}

int console_set(const struct console_s *new)
{
	cnsl_old = cnsl;
	cnsl = (new == NULL) ? cnsl_def : new;
	return 0;
}

int console_restore(void)
{
	const struct console_s *tmp;
	tmp = cnsl;
	cnsl = cnsl_old;
	cnsl_old = tmp;
	return 0;
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
