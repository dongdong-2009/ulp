/*
 *	dusk@2011 initial version
 *		interface: spi, 40Mhz, 16bit, CPOL=0(sck low idle), CPHA=0(latch at 1st edge/rising edge of sck), msb first
*/

#include "config.h"
#include "mbi5025.h"
#include "spi.h"

void mbi5025_Init(mbi5025_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.freq = 1000000,
	};
	chip->bus->init(&cfg);
}

void mbi5025_WriteByte(mbi5025_t *chip, unsigned char data)
{
	chip->bus->wreg(chip->idx, data);
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mbi5025_t sr = {
	.bus = &spi2,
	.idx = SPI_CS_DUMMY,
};

static int cmd_mbi5025_func(int argc, char *argv[])
{
	int temp = 0;

	const char * usage = { \
		" usage:\n" \
		" mbi5025 init, chip init \n" \
		" mbi5025 write value, write reg \n" \
	};

	if (argc > 1) {
		if(argv[1][0] == 'i')
			mbi5025_Init(&sr);

		if (argv[1][0] == 'w') {
			sscanf(argv[2], "%x", &temp);
			mbi5025_WriteByte(&sr, temp);
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_mbi5025 = {"mbi5025", cmd_mbi5025_func, "mbi5025 cmd"};
DECLARE_SHELL_CMD(cmd_mbi5025)
#endif
