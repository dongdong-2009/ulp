/*
 *	dusk@2011 initial version
 *		interface: spi, 40Mhz, 16bit, CPOL=0(sck low idle), CPHA=0(latch at 1st edge/rising edge of sck), msb first
*/

#include "config.h"
#include "74hct595.h"
#include "spi.h"

void nxp_74hct595_Init(nxp_74hct595_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.freq = 9000000,
	};
	chip->bus->init(&cfg);
}

void nxp_74hct595_WriteByte(nxp_74hct595_t *chip, unsigned char data)
{
	chip->bus->wreg(chip->idx, data);
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static nxp_74hct595_t nxp_74hct595 = {
	.bus = &spi1,
	.idx = SPI_1_NSS,
};

static int cmd_74hct595_func(int argc, char *argv[])
{
	int temp = 0;

	const char * usage = { \
		" usage:\n" \
		" 74hct595 init, chip init \n" \
		" 74hct595 write value, write reg \n" \
	};

	if (argc > 1) {
		if(argv[1][0] == 'i')
			nxp_74hct595_Init(&nxp_74hct595);

		if (argv[1][0] == 'w') {
			sscanf(argv[2], "%x", &temp);
			nxp_74hct595_WriteByte(&nxp_74hct595, temp);
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_74hct595 = {"74hct595", cmd_74hct595_func, "74hct595 cmd"};
DECLARE_SHELL_CMD(cmd_74hct595)
#endif
