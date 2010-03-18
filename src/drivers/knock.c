/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
		spi1: apb2/72Mhz, dds *3, dac, 36Mhz, CPOL=0(sck low idle), CPHA=1(latch at 2nd edge of sck),
		spi2: apb1/36Mhz, dpot*8, 9Mhz, CPOL=0(sck low idle), CPHA=0(latch at 1st edge of sck),
*/

#include "stm32f10x.h"
#include "knock.h"
#include "ad9833.h"
#include "mcp41x.h"

ad9833_t knock_dds = {
	.io = {
		.write_reg = 0,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SIN,
};

mcp41x_t knock_pot[NR_OF_KS] = {
	[0] = {
		.io = {
			.write_reg = 0,
			.read_reg = 0;
		},
	},

	[1] = {
		.io = {
			.write_reg = 0,
			.read_reg = 0;
		},
	},

	[2] = {
		.io = {
			.write_reg = 0,
			.read_reg = 0;
		},
	},

	[3] = {
		.io = {
			.write_reg = 0,
			.read_reg = 0;
		},
	},
};

void knock_Init(void)
{
	ad9833_Init(&knock_dds);
	mcp41x_Init(&knock_pot[KS1]);
	mcp41x_Init(&knock_pot[KS2]);
	mcp41x_Init(&knock_pot[KS3]);
	mcp41x_Init(&knock_pot[KS4]);
}

void knock_Update(void)
{
}

int knock_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_KNOCK_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&knock_dds, fw);
}

int knock_SetVolt(knock_ch_t ch, short mv)
{
	int ret = 0;
	int vout, pos;

	vout = mv;
	pos = (vout * 255) / CONFIG_DRIVER_KNOCK_MVPP_MAX;
	if(pos > 255) {
		ret = -1;
		pos = 255;
	}

	mcp41x_SetPos(&knock_pot[ch], pos);
	return 0;
}
