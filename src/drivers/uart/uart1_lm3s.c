/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "uart.h"
#include "lm3s.h"

static int uart_Init(const uart_cfg_t *cfg)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
	UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), cfg->baud, \
		(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	return 0;
}

static int uart_putchar(int data)
{
	char c = (char) data;
	UARTCharPut(UART1_BASE, c);
	return 0;
}

static int uart_getch(void)
{
	int ret;
	ret = UARTCharGet(UART1_BASE);
	return ret;
}

static int uart_IsNotEmpty()
{
	return (int) UARTCharsAvail(UART1_BASE);
}

uart_bus_t uart1 = {
	.init = uart_Init,
	.putchar = uart_putchar,
	.getchar = uart_getch,
	.poll = uart_IsNotEmpty,
};
