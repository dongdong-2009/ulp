/*
 * dusk@2011 initial version
 * note: this device doesn't not support "SPI CS Signal" in ULP Platform.
 *       and the cs signal should controlled in device layer
 */
#ifndef __MCP23S17_H_
#define __MCP23S17_H_

#include "spi.h"

//chip register define
#define ADDR_IODIRA	0x00
#define ADDR_IODIRB	0x01
#define ADDR_IOCON	0x0a
#define ADDR_GPPUA	0x0c
#define ADDR_GPPUB	0x0d
#define ADDR_GPIOA	0x12
#define ADDR_GPIOB	0x13

#define MCP23S17_HIGH_SPEED	0x01
#define MCP23S17_LOW_SPEED	0x02
#define MCP23017_PORTA_IN	0x04
#define MCP23017_PORTA_OUT	0x08
#define MCP23017_PORTB_IN	0x10
#define MCP23017_PORTB_OUT	0x20

typedef struct {
	const spi_bus_t *bus;
	int idx;
	int option;
} mcp23s17_t;

void mcp23s17_Init(mcp23s17_t *chip);
//data sequence like this: GPIOB in high byte, GIIOA in low byte
int mcp23s17_WriteByte(mcp23s17_t *chip, unsigned char addr, unsigned char data);
int mcp23s17_ReadByte(mcp23s17_t *chip, unsigned char addr, unsigned char *pdata);

#endif /*__MCP23S17_H_*/
