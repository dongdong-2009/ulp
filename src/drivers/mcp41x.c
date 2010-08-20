/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "mcp41x.h"
#include "spi.h"

#define CMD_WRITE_DATA (0x01 << 12)
#define CMD_SHUT_DOWN (0x02 << 12)
#define SEL_POT_0 (0x01 << 8)
#define SEL_POT_1 (0x02 << 8)
#define SEL_POT_BOTH (0x03 << 8)

void mcp41x_Init(mcp41x_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 16,
		.bseq = 1,
	};
	chip->bus->init(&cfg);
}

void mcp41x_SetPos(mcp41x_t *chip, short pos) /*0~255*/
{
	pos &= 0xff;
	chip->bus->wreg(chip->idx, CMD_WRITE_DATA | SEL_POT_0 | pos);
}
