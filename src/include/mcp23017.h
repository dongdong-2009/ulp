/*
 *	dusk@2010 initial version
 */

#ifndef __MCP23017_H
#define __MCP23017_H

#include "i2c.h"
typedef enum {
	MCP23017_PORT_IN = 0,
	MCP23017_PORT_OUT
} mcp23017_port_type;

typedef struct {
	i2c_bus_t *bus;
	unsigned char chip_addr;
	mcp23017_port_type port_type;	/*input or output*/
} mcp23017_t;

void mcp23017_Init(mcp23017_t *chip);
int mcp23017_WriteByte(mcp23017_t *chip, unsigned addr, int alen, unsigned char *buffer);
int mcp23017_ReadByte(mcp23017_t *chip, unsigned addr, int alen, unsigned char *buffer);

#endif /* __MCP23017_H */
