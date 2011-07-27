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
	};
	chip->bus->init(&cfg);
}

void nxp_74hct595_WriteByte(nxp_74hct595_t *chip, unsigned char data)
{
	chip->bus->wreg(chip->idx, data);
}
