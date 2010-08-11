/*
*	miaofng@2010 initial version
 *		ad9833, dds, max mclk=25MHz, 28bit phase resolution, 10bit DAC
 *		interface: spi, 40Mhz, 16bit, CPOL=1(sck high idle), CPHA=0(latch at 1st edge/falling edge of sck), msb first
*/

#include "ad9833.h"
#include "spi.h"

#define PHASE_RESOLUTION 28

#define REG_CTRL(val) ((0x00 << 14) | val)
#define REG_FREQ0(val) ((0x01 << 14) | val)
#define REG_FREQ1(val) ((0x02 << 14) | val)
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
	int opt = chip->option;
	chip->io.write_reg(0, REG_CTRL(RESET));
	
	opt |= B28;
	chip->io.write_reg(0, REG_CTRL(opt | RESET));
	chip->option = opt;
}

void ad9833_SetFreq(ad9833_t *chip, unsigned fw)
{
	unsigned short msb, lsb;
	int opt = chip->option;
	
	fw >>= (32 - PHASE_RESOLUTION);
	
	lsb = (unsigned short)(fw & 0x3fff); //low 14 bit
	fw >>= 14;
	msb = (unsigned short)(fw & 0x3fff);
	
	if(opt & FSELECT) {
		chip->io.write_reg(0, REG_FREQ0(lsb));
		chip->io.write_reg(0, REG_FREQ0(msb));
		opt &= ~FSELECT;
		chip->io.write_reg(0, REG_CTRL(opt));
	}
	else {
		chip->io.write_reg(1, REG_FREQ1(lsb));
		chip->io.write_reg(1, REG_FREQ1(msb));
		opt |= FSELECT;
		chip->io.write_reg(0, REG_CTRL(opt));
	}

	chip->option = opt;
}

static unsigned short ad9833_dam_buf0[3];
static unsigned short ad9833_dam_buf1[3];
static int flag_buf;

void ad9833_DMA_SetFreq(ad9833_t *chip, unsigned fw)
{
	unsigned short msb, lsb;
	int opt = chip->option;

	fw >>= (32 - PHASE_RESOLUTION);

	lsb = (unsigned short)(fw & 0x3fff); //low 14 bit
	fw >>= 14;
	msb = (unsigned short)(fw & 0x3fff);

	if(opt & FSELECT) {
		opt &= ~FSELECT;
		if (!flag_buf) {
			ad9833_dam_buf0[0] = REG_FREQ0(lsb);
			ad9833_dam_buf0[1] = REG_FREQ0(msb);
			ad9833_dam_buf0[2] = REG_CTRL(opt);
			flag_buf = 1;
			spi_DMA_Write(1, ad9833_dam_buf0, 3);
		} else {
			ad9833_dam_buf1[0] = REG_FREQ0(lsb);
			ad9833_dam_buf1[1] = REG_FREQ0(msb);
			ad9833_dam_buf1[2] = REG_CTRL(opt);
			flag_buf = 0;
			spi_DMA_Write(1, ad9833_dam_buf1, 3);
		}
	} else {
		opt |= FSELECT;
		if (!flag_buf) {
			ad9833_dam_buf0[0] = REG_FREQ1(lsb);
			ad9833_dam_buf0[1] = REG_FREQ1(msb);
			ad9833_dam_buf0[2] = REG_CTRL(opt);
			flag_buf = 1;
			spi_DMA_Write(1, ad9833_dam_buf0, 3);
		} else {
			ad9833_dam_buf1[0] = REG_FREQ1(lsb);
			ad9833_dam_buf1[1] = REG_FREQ1(msb);
			ad9833_dam_buf1[2] = REG_CTRL(opt);
			flag_buf = 0;
			spi_DMA_Write(1, ad9833_dam_buf1, 3);
		}
	}

	chip->option = opt;
}

void ad9833_Enable(const ad9833_t *chip)
{
	int opt = chip->option;
	chip->io.write_reg(0, REG_CTRL(opt));
}

void ad9833_Disable(const ad9833_t *chip)
{
	int opt = chip->option;
	chip->io.write_reg(0, REG_CTRL(opt | RESET));
}
