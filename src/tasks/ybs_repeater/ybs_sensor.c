/*
 * 	king@2012 initial version
 */

#include "ybs_sensor.h"
#include <stdint.h>
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_rtc.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x.h"
#include <string.h>

#define MUX_CHANNEL 0x001c0000		//p1[18-20]		multiplexer channel-choice pin
#define MUX_ENABLE 0x00200000		//p1[21]		multiplexer enable

static void sensor_choice_init()
{
	GPIO_SetDir(1, MUX_CHANNEL | MUX_ENABLE, 1);		//set p1[18-21] as output
	LPC_GPIO1->SET = MUX_ENABLE;				//disable multiplexer
}

static void sensor_choice(uint32_t sensor)
{
	sensor = sensor << 18;
	LPC_GPIO1->SET = MUX_ENABLE;				//disable multiplexer
	LPC_GPIO1->PIN = ((LPC_GPIO1->PIN) & (~(7 << 18))) | sensor ;
	LPC_GPIO1->CLR = MUX_ENABLE;				//enable multiplexer
}

static void ybs_uart_init()
{
	// UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;
	// UART FIFO configuration Struct variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

	/*
	 * Initialize UART2 pin connect
	 * P0.10: U2_TXD
	 * P0.11: U2_RXD
	 */
	PINSEL_ConfigPin(0,10,1);
	PINSEL_ConfigPin(0,11,1);
	/*
	 * Initialize UART3 pin connect
	 * P0.0: U3_TXD
	 * P0.1: U3_RXD
	 */
	PINSEL_ConfigPin(0,0,2);
	PINSEL_ConfigPin(0,1,2);

	UARTConfigStruct.Baud_rate = 115200;
	UARTConfigStruct.Databits = UART_DATABIT_8;
	UARTConfigStruct.Parity = UART_PARITY_NONE;
	UARTConfigStruct.Stopbits = UART_STOPBIT_1;
	UART_Init(LPC_UART2, &UARTConfigStruct);
	UART_Init(LPC_UART3, &UARTConfigStruct);

	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART2, &UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART3, &UARTFIFOConfigStruct);

	UART_TxCmd(LPC_UART2, ENABLE);
	UART_TxCmd(LPC_UART3, ENABLE);
}

void sensor_init()
{
	sensor_choice_init();
	ybs_uart_init();
}

void read_sensor(uint16_t *bank0, uint16_t *bank1)
{
	LPC_UART2->FCR |= 4;
	LPC_UART3->FCR |= 4;	//clear txbuf
	if(LPC_UART2->LSR & UART_LSR_RDR) {
		*bank0 = (uint16_t)(LPC_UART2->RBR & UART_RBR_MASKBIT);
		*bank0 = (*bank0 << 8) + (LPC_UART2->RBR & UART_RBR_MASKBIT);
	}
	else {
		*bank0 = NO_RESPONSE;
	}
	if(LPC_UART3->LSR & UART_LSR_RDR) {
		*bank1 = (uint16_t)(LPC_UART3->RBR & UART_RBR_MASKBIT);
		*bank1 = (*bank1 << 8) + (LPC_UART3->RBR & UART_RBR_MASKBIT);
	}
	else {
		*bank1 = NO_RESPONSE;
	}
}

void write_sensor(uint32_t sensor, uint8_t cmd)
{
	sensor_choice(sensor);
	LPC_UART2->FCR |= 2;
	LPC_UART3->FCR |= 2;	//clear rxbuf
	LPC_UART2->THR = cmd & UART_THR_MASKBIT;
	LPC_UART3->THR = cmd & UART_THR_MASKBIT;
}

void sensor_update()
{
}

