/*
*	miaofng@2010 initial version
 *		ad9833, dds, max mclk=25MHz, 28bit phase resolution, 10bit DAC
 *		interface: spi, 40Mhz, 16bit, CPOL=0(sck low idle), CPHA=1(latch at 2nd edge of sck)
*/

#include "ad9833.h"
//#include "normalize.h"

#define PHASE_RESOLUTION 28

#define REG_CTRL(val) ((0x00 << 14) | val)
#define REG_FREQ0(val) ((0x01 << 14) | val)
#define REG_FREQ1(val) ((0x02 << 14) | val))
#define REG_PHASE0(val) ((0x03 << 14) | (0x00 << 13) | val)
#define REG_PHASE1(val) ((0x03 << 14) | (0x01 << 13) | val)

/*ctrl reg bits*/
#define B28 (1 << 13) /*1, seq write 14LSB + 14MSB*/
#define HLB (1 << 12)
#define FSELECT (1 << 11) /*write freq0/1*/
#define PSELECT (1 << 10) /*write phase0/1*/
#define RESET (1 << 8)
#define SLEEP1 (1 << 7)
#define SLEEP12 (1 << 6)

void ad9833_Init(ad9833_t *chip)
{
	int priv = chip->option;
	chip->io.write_reg(0, REG_CTRL(RESET));
	
	priv |= B28;
	chip->io.write_reg(0, REG_CTRL(priv));
	chip->priv = priv;
}

void ad9833_SetFreq(ad9833_t *chip, unsigned fw)
{
	unsigned short msb, lsb;
	int priv = chip->priv;
	
	if((priv & AD9833_OPT_DIV) && (priv & AD9833_OPT_OUT_SQU))
		fw >>= (32 - PHASE_RESOLUTION - 1);
	else
		fw >>= (32 - PHASE_RESOLUTION);
	
	lsb = (unsigned short)(fw);
	fw >>= 16;
	msb = (unsigned short)(fw);
	
	if(priv & FSELECT) { /*current freq1 works*/
		chip->io.write_reg(0, REG_FREQ0(lsb));
		chip->io.write_reg(0, REG_FREQ0(msb));
		priv &= ~FSELECT;
		chip->io.write_reg(0, REG_CTRL(priv));
	}
	else { /*current freq0 works*/
		chip->io.write_reg(1, REG_FREQ0(lsb));
		chip->io.write_reg(1, REG_FREQ0(msb));
		priv |= FSELECT;
		chip->io.write_reg(0, REG_CTRL(priv));
	}
	
	chip->priv = priv;
}
