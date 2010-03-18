/*
 * 	miaofng@2010 initial version
  *		mcp41x, digi pot, 256taps, 10K/50K/100Kohm, single
 *		interface: spi, 10Mhz, 16bit, CPOL=0(sck low idle), CPHA=0(strobe at 1st edge of sck)
 */
#ifndef __MCP41X_H_
#define __MCP41X_H_

#include "device.h"

typedef struct {
	dev_io_t io;
} mcp41x_t;

void mcp41x_Init(mcp41x_t *chip);
int mcp41x_SetPos(mcp41x_t *chip, int pos); /*0~255*/

#endif /*__MCP41X_H_*/
