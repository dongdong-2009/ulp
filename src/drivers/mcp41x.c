/*
*	miaofng@2010 initial version
*/

#include "mcp41x.h"

#define CMD_WRITE_DATA (0x01 << 12)
#define CMD_SHUT_DOWN (0x02 << 12)
#define SEL_POT_0 (0x01 << 8)
#define SEL_POT_1 (0x02 << 8)
#define SEL_POT_BOTH (0x03 << 8)

void mcp41x_Init(mcp41x_t *chip)
{
}

void mcp41x_SetPos(mcp41x_t *chip, int pos) /*0~255*/
{
	chip->io.write_reg(0, CMD_WRITE_DATA | SEL_POT_0 | pos);
}
