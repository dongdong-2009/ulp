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

void aduc_adc_init(const aduc_adc_cfg_t *cfg)
{
	ADCMDE = (1 << 7) | ADUC_ADC_MODE_IDLE; //force adc to idle mode
	IEXCON = (1 << 6) | (cfg->iexc << 1) | cfg->ua10;
	ADCFLT = (1 << 7) | 127; //Fadc = 50Hz, chop off
	//ADCFLT = 0xc000 | (1 << 7) | 52;//Fadc = 50.3Hz, chop on
	ADCCFG = 0;

	if(cfg->adc0) {
		unsigned v = (1 << 15) | (1 << 11) | (cfg->mux0 << 6) | cfg->pga0;
		ADC0CON = v; /*enable ADC0*/
	}
	if(cfg->adc1) {
		unsigned v = (1 << 15) | (cfg->mux1 << 7) | cfg->pga1; /*enable ADC1*/
		ADC1CON = v;
	}

	/*zcal*/
	ADCMDE = (1 << 7) | ADUC_ADC_MODE_ZCAL_SELF; //self offset calibration
	while((ADCSTA & 0x8000) == 0);

	#if 0 /*gain self calibration only valid when PGA=0*/
	ADCMDE = (1 << 7) | ADUC_ADC_MODE_GCAL_SELF; //self gain calibration
	while((ADCSTA & 0x8000) == 0);
	printf("PGA = %d: ADC0OF = %d, ADC0GN = %d\n", cfg->pga0, ADC0OF, ADC0GN);
	#endif

	ADCMDE = (1 << 7) | ADUC_ADC_MODE_AUTO;
}

int aduc_adc_get(int adc, int *result)
{
	int ecode = 1; //busy
	aduc_adc_status_t status;
	status.value = ADCSTA;

	if(adc == 0) {
		if(status.ADC0RDY) {
			ecode = - status.ADC0CERR;
			*result = ADC0DAT;
		}
	}
	else {
		if(status.ADC1RDY) {
			ecode = - status.ADC1CERR;
			ADC0DAT; /*dummy*/
			*result = ADC1DAT;
		}
	}

	return ecode;
}
