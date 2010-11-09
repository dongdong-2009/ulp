/*
* tsc2046 is a 4-wire touch screen controller
* - low voltage, 2.2v ~ 5.25v core, 1.5~5.25v IO
* - internal 2.5Vref
* - direct battery measurement, 0~6v
* - on chip temperature measurement
* - touch pressure measurement
* - 125Khz(2MHz/16)/12bit ADC
* - QSPI/SPI: 2MHz, 24bit, CPOL=1(sck high idle), CPHA=1(latch at 2nd edge/rising edge of sck)
* - bit seq of miso: Start A2 A1 A0 MODE SER/DFR_ PD1 PD0 ----------------------------------------------
* - bit seq of mosi:  ----------------------------------------------------------0 bit11 bit10 bit9 bit8 .. bit0 0 0 0
*
*   miaofng@2010 initial version
*/

#include "config.h"
#include "spi.h"
#include "tsc2046.h"
#include "pd.h"
#include "stdio.h"
#include <string.h>

//#define _DEBUG

#define N 15
const static char obuf[N] = {
	START_BIT | CHD_PRZ1 | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Z1*/
	START_BIT | CHD_POSX | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos X*/
	START_BIT | CHD_POSY | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Y*/
	START_BIT | CHD_PRZ2 | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Z2*/
	START_BIT | CHD_PRZ1 | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Z1*/
};

int pd_dx;
int pd_dy;
int pd_zl;

static char ibuf[N];
static tsc2046_t tsc;

int tsc2046_init(const tsc2046_t *chip)
{
	const spi_bus_t *spi = chip -> bus;
	spi_cfg_t cfg = SPI_CFG_DEF;
	
	//spi init
	cfg.cpol = 1;
	cfg.cpha = 1;
	cfg.bits = 8;
	cfg.bseq = 1;
	cfg.freq = 2000000;
	spi -> init(&cfg);
	spi -> wbuf(obuf, ibuf, N);

	//save chip config for later usage
	memcpy(&tsc, chip, sizeof(tsc2046_t));
	
	//glvar init
	pd_dx = CONFIG_PD_DX;
	pd_dy = CONFIG_PD_DY;
	pd_zl = CONFIG_PD_ZL;
        return 0;
}

int pdd_get(struct pd_sample *sp)
{
	const spi_bus_t *spi = tsc.bus;
	int x1, y1, z1, z2, z3;
	int x, y, z = 0;
	int ret = -1;

	//spi data queue not finished?
	if(spi -> poll()) {
		return ret;
	}

#ifdef _DEBUG
	printf("tsc2046 ibuf dump:");
	for( int i = 0; i < N; i ++) {
		printf(" %02x", ibuf[i]);
	}
	printf("\n");
#endif

	//data process
	z1 = (int)(ibuf[3*0 + 1] << 8) + ibuf[3*0 + 2];
	x1 = (int)(ibuf[3*1 + 1] << 8) + ibuf[3*1 + 2];
	y1 = (int)(ibuf[3*2 + 1] << 8) + ibuf[3*2 + 2];
	z2 = (int)(ibuf[3*3 + 1] << 8) + ibuf[3*3 + 2];
	z3 = (int)(ibuf[3*4 + 1] << 8) + ibuf[3*4 + 2];

	if( z1 > 0x200 && z2 > 0x200) {
		x = x1;
		y = y1;
		z1 = (z1 + z3) >> 1;

		z = z2 - z1; //max 2^15
		z *= x; //max 2^30
		z >>= 15; //max 2^15
		z *= CONFIG_PD_RX;
		z /= z1;

		//save result
		if(z < pd_zl) {
			sp -> x = x;
			sp -> y = y;
			sp -> z = z;
			ret = 0;
		}
	}

	//restart queue
	spi -> wbuf(obuf, ibuf, N);
	return ret;
}
