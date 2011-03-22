/*
 *	dusk@2010 initial version
 */

#include "i2c.h"
#include "mcp23017.h"

//chip register define
#define IODIRA	0x00
#define IODIRB	0x01
#define IOCON	0x0a
#define GPPUA	0x0c
#define GPPUB	0x0d

void mcp23017_Init(mcp23017_t *chip)
{
	unsigned char temp = 0x28;
	i2c_cfg_t cfg = {
		40000,		//40K
		NULL,
	};

	chip->bus->init(&cfg);

	//bank=0, chip address enable.
	chip->bus->wreg(chip->chip_addr, IOCON, 0x01, &temp);

	if (chip->port_type == MCP23017_PORT_IN) {
		temp = 0xff;
		chip->bus->wreg(chip->chip_addr, IODIRA, 0x01, &temp);
		chip->bus->wreg(chip->chip_addr, IODIRB, 0x01, &temp);

		temp = 0xff;
		chip->bus->wreg(chip->chip_addr, GPPUA, 0x01, &temp);
		chip->bus->wreg(chip->chip_addr, GPPUB, 0x01, &temp);
	} else {
		temp = 0x00;
		chip->bus->wreg(chip->chip_addr, IODIRA, 0x01, &temp);
		chip->bus->wreg(chip->chip_addr, IODIRB, 0x01, &temp);
	}
}

int mcp23017_WriteByte(mcp23017_t *chip, unsigned addr, int alen, unsigned char *buffer)
{
	return chip->bus->wreg(chip->chip_addr, addr, alen, buffer);
}

int mcp23017_ReadByte(mcp23017_t *chip, unsigned addr, int alen, unsigned char *buffer)
{
	return chip->bus->rbuf(chip->chip_addr, addr, alen, buffer, 1);
}

#if 0
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mcp23017_t mcp23017 = {
	.bus = &i2c1,
	.chip_addr = 0x40,
	.port_type = MCP23017_PORT_IN
};

static int cmd_mcp23017_func(int argc, char *argv[])
{
	unsigned int temp = 0;
	unsigned int addr;

	const char * usage = { \
		" usage:\n" \
		" mcp23017 init, chip init \n" \
		" mcp23017 write addr value, write reg \n" \
		" mcp23017 read addr, read reg\n" \
		" mcp23017 read repeat addr, repeat read reg\n" \
	};

	if (argc > 1) {
		if(argv[1][0] == 'i')
			mcp23017_Init(&mcp23017);

		if (argv[1][0] == 'w') {
			sscanf(argv[2], "%x", &addr);
			sscanf(argv[3], "%x", &temp);
			mcp23017_WriteByte(&mcp23017, addr, 1, (unsigned char *)&temp);
		}

		if (argv[1][0] == 'r' && argc == 3) {
			sscanf(argv[2], "%x", &addr);
			mcp23017_ReadByte(&mcp23017, addr, 1, (unsigned char *)&temp);
			printf("%c = 0x%x\n",(unsigned char)temp, (unsigned char)temp);
		}
	}

	if (argc == 0 || argv[2][0] == 'r') {
		//sscanf(argv[3], "%x", &addr);
		mcp23017_ReadByte(&mcp23017, 0x12, 1, (unsigned char *)&temp);
		printf("0x%x\n", (unsigned char)temp);
		return 1;
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_mcp23017 = {"mcp23017", cmd_mcp23017_func, "mcp23017 cmd"};
DECLARE_SHELL_CMD(cmd_mcp23017)
#endif
