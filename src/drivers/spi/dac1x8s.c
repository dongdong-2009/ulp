/*
*	miaofng@2010 initial version
 *		dac1x8s085,  8ch,8bit/10bit/12bit, 6uS settling time, rail-to-rail
 *		interface: spi, 40Mhz, 16bit, CPOL=1(sck high idle), CPHA=0(latch at 1st edge/falling edge of sck), msb first
*/

#include "config.h"
#include "dac1x8s.h"
#include "spi.h"

#define CMD_MOD_WRM		0x8000
#define CMD_MOD_WTM		0x9000
#define CMD_UPD_SEL		0xa000
#define CMD_UPD_CHA		0xb000
#define CMD_UPD_ALL		0xc000
#define CMD_OUT_HIZ		0xd000
#define CMD_OUT_100K	0xe000
#define CMD_OUT_2p5K	0xf000

void dac1x8s_Init(const dac1x8s_t *chip)
{
	spi_cfg_t cfg = {
		.cpol = 1,
		.cpha = 0,
		.bits = 16,
		.bseq = 1,
	};
	chip->bus->init(&cfg);
	chip->bus->wreg(chip->idx, CMD_MOD_WTM);
}

void dac1x8s_SetVolt(const dac1x8s_t *chip, int ch, int cnt)
{
	ch &= 0x0007;
	cnt &= 0x0fff;
	cnt |= ch << 12;
	chip->bus->wreg(chip->idx, cnt);
}
