/*
 *	dusk@2011 initial version
 *		interface: spi, 40Mhz, 16bit, CPOL=0(sck low idle), CPHA=0(latch at 1st edge/rising edge of sck), msb first
*/

#include "config.h"
#include "mbi5025.h"
#include "spi.h"

void mbi5025_Init(const mbi5025_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.csel = 1,
		.freq = 1000000,
	};
	chip->bus->init(&cfg);
}

void mbi5025_WriteByte(const mbi5025_t *chip, unsigned char data)
{
	chip->bus->wreg(chip->idx, data);
}

void mbi5025_WriteBytes(const mbi5025_t *chip, unsigned char * pdata, int len)
{
	int i;
	for (i = 0; i < len; i++)
		chip->bus->wreg(chip->idx, *(pdata++));
	spi_cs_set(chip->load_pin, 1);
	spi_cs_set(chip->load_pin, 0);
}

void mbi5025_EnableLoad(const mbi5025_t *chip)
{
	spi_cs_set(chip->load_pin, 1);
}

void mbi5025_DisableLoad(const mbi5025_t *chip)
{
	spi_cs_set(chip->load_pin, 0);
}

void mbi5025_EnableOE(const mbi5025_t *chip)
{
	spi_cs_set(chip->oe_pin, 0);
}

void mbi5025_DisableOE(const mbi5025_t *chip)
{
	spi_cs_set(chip->oe_pin, 1);
}

void mbi5025_write_and_latch(const mbi5025_t *chip, const void *p, int n)
{
	const char *pbyte = p;
	spi_cs_set(chip->load_pin, 0);
	while(n > 0) {
		char byte = *pbyte ++;
		chip->bus->wreg(0, byte);
		n --;
	}
	spi_cs_set(chip->load_pin, 1);
	spi_cs_set(chip->load_pin, 0);
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mbi5025_t sr = {
	.bus = &spi1,
	.idx = SPI_CS_DUMMY,
	.load_pin = SPI_CS_PC3,
	.oe_pin = SPI_CS_PC4,
};

static int cmd_mbi5025_func(int argc, char *argv[])
{
	int temp = 0;
	int i;

	const char * usage = { \
		" usage:\n" \
		" mbi5025 init, chip init \n" \
		" mbi5025 write v1 v2 ...., write reg with 0xv1 0xv2... \n" \
	};

	if (argc > 1) {
		if(argv[1][0] == 'i') {
			mbi5025_Init(&sr);
			mbi5025_EnableOE(&sr);
			printf("Init Successful!\n");
		}

		if (argv[1][0] == 'w') {
			for (i = 0; i < (argc - 2); i++) {
				sscanf(argv[2+i], "%x", &temp);
				mbi5025_WriteByte(&sr, temp);
			}
			spi_cs_set(sr.load_pin, 1);
			spi_cs_set(sr.load_pin, 0);
			printf("%d Bytes Write Successful!\n", argc-2);
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
