/*
 * 	miaofng@2013-2-20 initial version
 *
 *	ADC1: 24bit, PGA=1..512
 *	ADC2: 24bit, PGA=1..8
 *	IREF: 200uA*N, N=1:5, 10uA opt
 *	Rref: 100Ohm
 *	VRref: 20mV*N(2mV@10uA)
 *
 */
#include "ulp/sys.h"
#include "aduc706x.h"
#include "aduc_adc.h"
#include "common/ansi.h"

#define __DEBUG(X) X
#define CONFIG_AVERAGE_FACTOR 0 /*1 << N times*/

#define ADC0OF_DEF 0x0000
#define ADC1OF_DEF 0x0000
#define ADC1OF_DEF 0x0000
#define ADC0GN_DEF 0x5552
#define ADC1GN_DEF 0x554B

static int __aduc_adc_init(const aduc_adc_cfg_t *cfg)
{
	ADCMDE = (1 << 7) | ADUC_ADC_MODE_IDLE; //force adc to idle mode
	IEXCON = (1 << 6) | (cfg->iexc << 1) | cfg->ua10;
#if CONFIG_ADUC_50HZ
	ADCFLT = (1 << 7) | 127; //Fadc = 50Hz, chop off
#endif
#if CONFIG_ADUC_50HZ_CHOP
	ADCFLT = 0xc000 | (1 << 7) | 52;//Fadc = 50.3Hz, chop on
#endif
#if CONFIG_ADUC_10HZ_CHOP
	ADCFLT = 0xc000 | (6 << 8) | (1 << 7) | 60;//AF=8, SF=60, Fadc = 11.9Hz, chop on
#endif

	ADC0CON = 0x00; //disable both adc first
	ADC1CON = 0x00;
	ADCCFG = 0;

	int mv = cfg->vofs;
	if(mv >= 1000) {
		DACCON = (1 << 4) | 0x03; //12bit mode, vref=AVDD=2V5
		mv = mv * 4095 / 2500;
		DACDAT = mv << 16;
	}
	else if(mv > 0) {
		DACCON = (1 << 4) | 0x00; //12bit mode, vref=1.2v
		mv = mv * 4095 / 1200;
		DACDAT = mv << 16;
	}

	if(cfg->adc0) {
		unsigned v = (1 << 15) | (1 << 11) | (cfg->mux0 << 6) | cfg->pga0;
		ADC0CON = v; /*enable ADC0*/
	}
	if(cfg->adc1) {
		unsigned v = (1 << 15) | (cfg->mux1 << 7) | (0x03 << 2) | cfg->pga1; /*enable ADC1*/
		ADC1CON = v;
	}

	ADCMDE = (1 << 7) | ADUC_ADC_MODE_AUTO;
	return 0;
}

int aduc_adc_init(const aduc_adc_cfg_t *cfg, int cal)
{
	int i, x, y, sum_x = 0, sum_y = 0, avg_x, avg_y;
	static short adc0of = ADC0OF_DEF;
	static short adc1of = ADC1OF_DEF;
	static short adc0gn = ADC0GN_DEF;
	static short adc1gn = ADC1GN_DEF;

	if(cal != ADUC_CAL_NONE) {
		__aduc_adc_init(cfg);
		mdelay(1); //without this delay, cal may lead to random result
	}

	/*zcal*/
	if((cal == ADUC_CAL_ZERO) || (cal == ADUC_CAL_FULL)) {
		for(i = 0; i < 5; i ++) {
			ADCMDE = (1 << 7) | ADUC_ADC_MODE_ZCAL_SELF; //self offset calibration
			while((ADCSTA & 0x01) == 0);
			mdelay(1);

			x = (cfg->adc0) ? ADC0OF : 0;
			y = (cfg->adc1) ? ADC1OF : 0;
			__DEBUG(printf("CAL: ADC0OF = 0x%08x ADC1OF = 0x%08x\n", x, y));

			avg_x = (avg_x * i + x) / (i + 1);
			avg_y = (avg_y * i + y) / (i + 1);
			sum_x += (avg_x > x) ? (avg_x - x) : (x - avg_x);
			sum_y += (avg_y > y) ? (avg_y - y) : (y - avg_y);
		}

		if(sum_x + sum_y < 10) { //success
			adc0of = (short) avg_x;
			adc1of = (short) avg_y;
		}
		else {
			__DEBUG(printf(ANSI_FONT_RED));
			__DEBUG(printf("CAL: FAIL(sum_x = %d, sum_y = %d)!!!\n", sum_x, sum_y));
			__DEBUG(printf(ANSI_FONT_DEF));
			goto EXIT_CAL;
		}
	}

	/*note: gain self calibration only valid when PGA=0!!!*/
	if((cal == ADUC_CAL_GAIN) || (cal == ADUC_CAL_FULL)) {
		for(i = 0; i < 5; i ++) {
			ADCMDE = (1 << 7) | ADUC_ADC_MODE_GCAL_SELF; //self gain calibration
			while((ADCSTA & 0x01) == 0);
			mdelay(1);

			x = (cfg->adc0) ? ADC0GN : 0;
			y = (cfg->adc1) ? ADC1GN : 0;
			__DEBUG(printf("CAL: ADC0GN = 0x%08x ADC1GN = 0x%08x\n", x, y));

			avg_x = (avg_x * i + x) / (i + 1);
			avg_y = (avg_y * i + y) / (i + 1);
			sum_x += (avg_x > x) ? (avg_x - x) : (x - avg_x);
			sum_y += (avg_y > y) ? (avg_y - y) : (y - avg_y);
		}

		if(sum_x + sum_y < 10) { //success
			adc0gn = (short) avg_x;
			adc1gn = (short) avg_y;
		}
		else {
			__DEBUG(printf(ANSI_FONT_RED));
			__DEBUG(printf("CAL: FAIL(sum_x = %d, sum_y = %d)!!!\n", sum_x, sum_y));
			__DEBUG(printf(ANSI_FONT_DEF));
			goto EXIT_CAL;
		}
	}


EXIT_CAL:
	__aduc_adc_init(cfg);
	ADC0OF = adc0of;
	ADC1OF = adc1of;
	ADC0GN = adc0gn;
	ADC1GN = adc1gn;

	__DEBUG(printf("CAL: ADC0OF = 0x%08x ADC1OF = 0x%08x\n", ADC0OF, ADC1OF));
	__DEBUG(printf("CAL: ADC0GN = 0x%08x ADC1GN = 0x%08x\n", ADC0GN, ADC1GN));
	return (sum_x + sum_y < 10) ? 0 : -1;
}

int aduc_adc_is_ready(int adc)
{
	aduc_adc_status_t status;
	status.value = ADCSTA;
	return (adc == 0) ? status.ADC0RDY : status.ADC1RDY;
}

//ADCxRDY is set in case of over-range error???
int aduc_adc_get(int *adc0, int *adc1)
{
	if(adc0 != NULL) {
		while(! aduc_adc_is_ready(0));
	}

	if(adc1 != NULL) {
		while(! aduc_adc_is_ready(1));
	}

	aduc_adc_status_t status;
	status.value = ADCSTA;

	int v0, v1, ecode = 0;
	v0 = ADC0DAT;
	v1 = ADC1DAT;
	if(adc0 != NULL) {
		*adc0 = v0;
		ecode += (status.ADC0CERR) ? -1 : 0;
	}

	if(adc1 != NULL) {
		*adc1 = v1;
		ecode += (status.ADC1CERR) ? -1 : 0;
	}

	return ecode;
}
