/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "uart.h"
#include "sam3u.h"

static int uart_Init(const uart_cfg_t *cfg)
{
	// Enable clock for UART
	AT91C_BASE_PMC -> PMC_PCER = (1 << AT91C_ID_DBGU);

	// Pin Function Config PA11 | PA12
	int mask = (1 << 11) | (1 << 12);
	AT91C_BASE_PIOA -> PIO_IDR = mask; // irq disable
	AT91C_BASE_PIOA -> PIO_PPUDR = mask; //pull up disable
	int abmr = AT91C_BASE_PIOA -> PIO_ABSR;
	AT91C_BASE_PIOA -> PIO_ABSR &= (~mask & abmr);
	AT91C_BASE_PIOA -> PIO_PDR = mask; //disable
	
	// Reset & disable receiver and transmitter, disable interrupts
	AT91C_BASE_DBGU -> DBGU_CR = AT91C_US_RSTRX | AT91C_US_RSTTX;
	AT91C_BASE_DBGU -> DBGU_IDR = 0xFFFFFFFF;
	AT91C_BASE_DBGU -> DBGU_BRGR = CONFIG_MCK_VALUE / (cfg -> baud * 16);
	AT91C_BASE_DBGU -> DBGU_MR = AT91C_US_PAR_NONE; // Configure mode register
	AT91C_BASE_DBGU -> DBGU_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS; // Disable DMA channel
	AT91C_BASE_DBGU -> DBGU_CR = AT91C_US_RXEN | AT91C_US_TXEN; // Enable receiver and transmitter
	return 0;
}

static int uart_putchar(int data)
{
	while ((AT91C_BASE_DBGU -> DBGU_CSR & AT91C_US_TXEMPTY) == 0);
	AT91C_BASE_DBGU -> DBGU_THR = data;
	return 0;
}

static int uart_getch(void)
{
	int ret;
	while ((AT91C_BASE_DBGU -> DBGU_CSR & AT91C_US_RXRDY) == 0);
	ret = AT91C_BASE_DBGU -> DBGU_RHR;
	return ret;
}

static int uart_IsNotEmpty()
{
	return (AT91C_BASE_DBGU -> DBGU_CSR & AT91C_US_RXRDY);
}

uart_bus_t uartg = {
	.init = uart_Init,
	.putchar = uart_putchar,
	.getchar = uart_getch,
	.poll = uart_IsNotEmpty,
};
