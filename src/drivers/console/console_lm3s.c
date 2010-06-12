/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "console.h"
#include "lm3s.h"

void console_Init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, \
		(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

int console_putchar(char c)
{
	UARTCharPut(UART0_BASE, c);
	if(c == '\n') {
		UARTCharPut(UART0_BASE, '\r');
	}
	return 0;
}

int console_getch(void)
{
	int ret;
	ret = UARTCharGet(UART0_BASE);
	return ret;
}

int console_getchar(void)
{
	int ret = console_getch();
	console_putchar((char)ret);
	return ret;
}

int console_IsNotEmpty()
{
	return (int) UARTCharsAvail(UART0_BASE);
}