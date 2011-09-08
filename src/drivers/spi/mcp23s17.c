/*
 *	dusk@2011 initial version
 *	interface: CPOL=0(sck low idle), CPHA=0(latch at 1st edge/rising edge of sck), msb first
*/

#include "config.h"
#include "mcp23s17.h"
#include "spi.h"

#define OPCODE_WRITE	0x40
#define OPCODE_READ		0x41

void mcp23s17_Init(mcp23s17_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.freq = 9000000,
	};
	if (chip->option & MCP23S17_HIGH_SPEED) {
		cfg.freq = 9000000;		//9MHz
	} else {
		cfg.freq = 500000;		//500KHz
	}
	chip->bus->init(&cfg);

	//ioconfig reg config
	mcp23s17_WriteByte(chip, ADDR_IOCON, 0x00);

	//porta config
	if (chip->option & MCP23017_PORTA_OUT) {
		mcp23s17_WriteByte(chip, ADDR_IODIRA, 0x00);
	} else {
		mcp23s17_WriteByte(chip, ADDR_IODIRA, 0xff);
		mcp23s17_WriteByte(chip, ADDR_GPPUA, 0xff);
	}

	//portb config
	if (chip->option & MCP23017_PORTB_OUT) {
		mcp23s17_WriteByte(chip, ADDR_IODIRB, 0x00);
	} else {
		mcp23s17_WriteByte(chip, ADDR_IODIRB, 0xff);
		mcp23s17_WriteByte(chip, ADDR_GPPUB, 0xff);
	}
}
int mcp23s17_WriteByte(mcp23s17_t *chip, unsigned char addr, unsigned char data)
{
	spi_cs_set(chip->idx, 0);
	chip->bus->wreg(chip->idx, OPCODE_WRITE);
	chip->bus->wreg(chip->idx, addr);
	chip->bus->wreg(chip->idx, data);
	spi_cs_set(chip->idx, 1);

	return 0;
}

int mcp23s17_ReadByte(mcp23s17_t *chip, unsigned char addr, unsigned char *pdata)
{
	spi_cs_set(chip->idx, 0);
	chip->bus->wreg(chip->idx, OPCODE_READ);
	chip->bus->wreg(chip->idx, addr);
	*pdata = chip->bus->rreg(chip->idx);
	spi_cs_set(chip->idx, 1);

	return 0;
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mcp23s17_t mcp23s17 = {
	.bus = &spi2,
	.idx = SPI_CS_PD12,
	.option = MCP23S17_LOW_SPEED | MCP23017_PORTA_OUT | MCP23017_PORTB_OUT,
};

static int cmd_mcp23s17_func(int argc, char *argv[])
{
	unsigned int temp = 0;
	unsigned int addr;

	const char * usage = { \
		" usage:\n" \
		" mcp23s17 init, chip init \n" \
		" mcp23s17 write addr value, write reg \n" \
		" mcp23s17 read addr, read reg\n" \
		" mcp23s17 read repeat addr, repeat read reg\n" \
	};

	if (argc > 1) {
		if (argv[1][0] == 'i') {
			mcp23s17_Init(&mcp23s17);
		}

		if (argv[1][0] == 'w') {
			sscanf(argv[2], "%x", &addr);
			sscanf(argv[3], "%x", &temp);
			mcp23s17_WriteByte(&mcp23s17, addr, temp);
		}

		if (argv[1][0] == 'r' && argc == 3) {
			sscanf(argv[2], "%x", &addr);
			mcp23s17_ReadByte(&mcp23s17, addr, (unsigned char *)&temp);
			printf("%c = 0x%x\n",(unsigned char)temp, (unsigned char)temp);
		}
	}

	if (argc == 0 || argv[2][0] == 'r') {
		//sscanf(argv[3], "%x", &addr);
		mcp23s17_ReadByte(&mcp23s17, addr, (unsigned char *)&temp);
		printf("0x%x\n", (unsigned char)temp);
		return 1;
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_mcp23s17 = {"mcp23s17", cmd_mcp23s17_func, "mcp23s17 cmd"};
DECLARE_SHELL_CMD(cmd_mcp23s17)
#endif
