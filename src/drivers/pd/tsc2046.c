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

#define N 18
const static char obuf[N] = {
	START_BIT | CHD_POSX | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos X*/
	START_BIT | CHD_POSY | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Y*/
	START_BIT | CHD_PRZ1 | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Z1*/
	START_BIT | CHD_PRZ2 | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Z2*/
	START_BIT | CHD_POSX | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos X*/
	START_BIT | CHD_POSY | ADC_12BIT | PD_OFF, 0x00, 0x00, /*measure pos Y*/
};

static char ibuf[N];
static tsc2046_t tsc;

int tsc2046_init(const tsc2046_t *chip)
{
	const spi_bus_t *spi;
	spi_cfg_t cfg = SPI_CFG_DEF;
	cfg.cpol = 1;
	cfg.cpha = 1;
	cfg.bits = 8;
	cfg.bseq = 1;
	cfg.freq = 4000000;

	memcpy(&tsc, chip, sizeof(tsc2046_t));
	spi = tsc.bus;
	spi -> init(&cfg);
	spi -> wbuf(obuf, ibuf, N);
        return 0;
}

int pdd_get(int *px, int *py)
{
	const spi_bus_t *spi = tsc.bus;
	int x1, x2, y1, y2, z1, z2;
	int x, y, z = 0;

	//spi data queue not finished?
	if(spi -> poll()) {
		return -1;
	}

	//data process
	x1 = (int)(ibuf[3*0 + 1] << 8) + ibuf[3*0 + 2];
	y1 = (int)(ibuf[3*1 + 1] << 8) + ibuf[3*1 + 2];
	z1 = (int)(ibuf[3*2 + 1] << 8) + ibuf[3*2 + 2] + 1; //avoid z2/z1 = z2/0 situation
	z2 = (int)(ibuf[3*3 + 1] << 8) + ibuf[3*3 + 2];
	x2 = (int)(ibuf[3*4 + 1] << 8) + ibuf[3*4 + 2];
	y2 = (int)(ibuf[3*5 + 1] << 8) + ibuf[3*5 + 2];

	x = (x1 > x2) ? (x1 - x2) : (x2 - x1);
	y = (y1 > y2) ? (y1 - y2) : (y2 - y1);
	if( x < pd_dx && y < pd_dy) {
		x = (x1 + x2) >> 1;
		y = (y1 + y2) >> 1;

		z = z2 - z1; //max 2^15
		z *= x; //max 2^30
		z >>= 15; //max 2^15
		z *= CONFIG_PD_RX;
		z /= z1;

		//save result
		if(z < pd_zl) {
			*px = x;
			*py = y;
		}
	}

#ifdef _DEBUG
	printf("tsc2046 ibuf dump:");
	for( int i = 0; i < N; i ++) {
		printf(" %02x", ibuf[i]);
	}
	if(z > 0 && z < pd_zl) {
		printf(" Z(Rt/Rx): %d", z);
	}
	printf("\n");
#endif

	//restart queue
	spi -> wbuf(obuf, ibuf, N);
	return z;
}
