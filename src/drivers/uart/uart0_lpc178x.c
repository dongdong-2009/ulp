/*
 *	miaofng@2012 refer to nxp example Uart_Polling.c
 */

#include "config.h"
#include "uart.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"

#define UART_TEST_NUM		0

#if (UART_TEST_NUM == 0)
#define	_LPC_UART		LPC_UART0
#define _UART_IRQ		UART0_IRQn
#define _UART_IRQHander		UART0_IRQHandler
#elif (UART_TEST_NUM == 1)
#define _LPC_UART		LPC_UART1
#define _UART_IRQ		UART1_IRQn
#define _UART_IRQHander		UART1_IRQHandler
#elif (UART_TEST_NUM == 2)
#define _LPC_UART		LPC_UART2
#define _UART_IRQ		UART2_IRQn
#define _UART_IRQHander		UART2_IRQHandler
#endif

static int uart_Init(const uart_cfg_t *cfg)
{
	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
#if (UART_TEST_NUM == 0)
	/*
	 * Initialize UART0 pin connect
	 * P0.2: U0_TXD
	 * P0.3: U0_RXD
	 */
	PINSEL_ConfigPin(0,2,1);
	PINSEL_ConfigPin(0,3,1);
#elif (UART_TEST_NUM == 1)
	/*
	 * Initialize UART1 pin connect
	 * P0.15: U1_TXD
	 * P0.16: U1_RXD
	 */
	PINSEL_ConfigPin(0,15,1);
	PINSEL_ConfigPin(0,16,1);
#elif (UART_TEST_NUM == 2)
	/*
	 * Initialize UART2 pin connect
	 * P0.10: U2_TXD
	 * P0.11: U2_RXD
	 */
	PINSEL_ConfigPin(0,10,1);
	PINSEL_ConfigPin(0,11,1);
#endif

	/* Initialize UART Configuration parameter structure to default state:
	 * Baudrate = 9600bps
	 * 8 data bit
	 * 1 Stop bit
	 * None parity
	 */
	UART_ConfigStructInit(&UARTConfigStruct);
	UARTConfigStruct.Baud_rate = cfg->baud;
	UART_Init(_LPC_UART, &UARTConfigStruct);

	/* Initialize FIFOConfigStruct to default state:
	 *  - FIFO_DMAMode = DISABLE
	 *  - FIFO_Level = UART_FIFO_TRGLEV0
	 *  - FIFO_ResetRxBuf = ENABLE
	 *  - FIFO_ResetTxBuf = ENABLE
	 *  - FIFO_State = ENABLE
	 */
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(_LPC_UART, &UARTFIFOConfigStruct);
	UART_TxCmd(_LPC_UART, ENABLE);
	return 0;
}

static int uart_putchar(int data)
{
	while(!(_LPC_UART->LSR & UART_LSR_THRE));
	_LPC_UART->THR = data & 0xff;
	return 0;
}

static int uart_IsNotEmpty(void)
{
	int ret;
	ret = _LPC_UART->LSR & UART_LSR_RDR;
	return ret;
}

static int uart_getch(void)
{
	int ret;
	while(!uart_IsNotEmpty());
	ret = _LPC_UART->RBR;
	return ret;
}

uart_bus_t uart0 = {
	.init = uart_Init,
	.putchar = uart_putchar,
	.getchar = uart_getch,
	.poll = uart_IsNotEmpty,
#ifdef CONFIG_UART_KWP_SUPPORT
	.wake = NULL,
#endif
};
