/*
 * Author    : dusk
 * miaofng@2010, api update to new format
 */

#ifndef __UART_H_
#define __UART_H_

//baudrate
enum {
	BAUD_9600 = 9600,
	BAUD_115200 = 115200,
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
	int (*isready)(void); //1 or more char has been received
} uart_bus_t;

extern uart_bus_t uart1;
extern uart_bus_t uart2;

#endif /* __UART_H_ */
