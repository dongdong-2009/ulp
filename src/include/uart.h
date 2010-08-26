/*
 * Author    : dusk
 * miaofng@2010, api update to new format
 */

#ifndef __UART_H_
#define __UART_H_

#include "config.h"

//baudrate
enum {
	BAUD_9600 = 9600,
	BAUD_115200 = 115200,
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

typedef struct {
	int (*init)(const uart_cfg_t *cfg);
	int (*putchar)(int data);
	int (*getchar)(void);
	int (*poll)(void); //return how many chars left in the rx buffer
	
#ifdef CONFIG_UART_KWP_SUPPORT
	int (*wake)(int op);
#endif
} uart_bus_t;

extern uart_bus_t uart0;
extern uart_bus_t uart1;
extern uart_bus_t uart2;

#endif /* __UART_H_ */
