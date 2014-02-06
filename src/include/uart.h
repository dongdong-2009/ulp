/*
 * Author    : dusk
 * miaofng@2010, api update to new format
 */

#ifndef __UART_H_
#define __UART_H_

#include "config.h"

//baudrate
enum {
	BAUD_300 = 300,
	BAUD_1200 = 1200,
	BAUD_2400 = 2400,
	BAUD_4800 = 4800,
	BAUD_9600 = 9600,
	BAUD_115200 = 115200,
	BAUD_230400 = 230400,
	BAUD_460800 = 460800,
	BAUD_921600 = 921600,
};

//wake ops, special support for keyword fast init protocol
enum {
	WAKE_EN, //enable goio drive mode
	WAKE_LO,
	WAKE_HI,
	WAKE_RS, //reset to normal uart mode
};

typedef struct {
	unsigned baud;
} uart_cfg_t;

#define UART_CFG_DEF { \
	.baud = BAUD_115200, \
}

typedef const struct {
	int (*init)(const uart_cfg_t *cfg);
	int (*putchar)(int data);
	int (*getchar)(void);
	int (*poll)(void); //return how many chars left in the rx buffer

#ifdef CONFIG_UART_KWP_SUPPORT
	int (*wake)(int op);
#endif
} uart_bus_t;

extern uart_bus_t uartg;
extern uart_bus_t uart0;
extern uart_bus_t uart1;
extern uart_bus_t uart2;
extern uart_bus_t uart3;

static inline void uart_send(uart_bus_t *uart, const void *frame, int n) {
	const char *p = frame;
	while(n > 0) {
		uart->putchar(*p ++);
		n --;
	}
}

static inline void uart_puts(uart_bus_t *uart, char *str)
{
	while(*str) uart->putchar(*str ++);
}

static inline void uart_flush(uart_bus_t *uart)
{
	while(uart->poll()) uart->getchar();
}

#endif /* __UART_H_ */
