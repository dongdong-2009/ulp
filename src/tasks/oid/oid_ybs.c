/*
*	miaofng@2013 ybs demo program based on oid board
*/
#include "config.h"
#include "ulp/sys.h"
#include "aduc706x.h"

void ybs_init(void)
{
	IEXCON = (5 << 1) | (1 << 6); //enable pin IEXC0 = 1mA
	ADCMDE = (1 << 7) | 3; //force adc to idle mode
	mdelay(1);
	ADC0CON = (1 << 15) | 9; //primary ch enable(ADC0/ADC1, internal 1.2vref, PGA = 512)
	//ADCFLT = 0xc000 | (1 << 7) | 52;//Fadc = 50.3Hz, chop on
	ADCFLT = (1 << 7) | 127; //Fadc = 50Hz, chop off
	ADCCFG = 0;
	ADCMDE = (1 << 7) | 4; //self offset calibration
	//ADCMDE = (1 << 7) | 6; //system offset calibration
	while((ADCSTA & 0x8000) == 0);
	//ADCMDE = (1 << 7) | 5; //self gain calibration
	//while((ADCSTA & 0x8000) == 0);
	ADCMDE = (1 << 7) | 1; //put adc to continous conversion mode
}

void ybs_update(void)
{
	if(ADCSTA & 0x01) {
		int v = ADC0DAT;
		v *= ((1200 * 1000 * 1000) >> 23);
		v >>= 9;
		printf("%03d : %d nv %08x\n", time_get(0) % 1000, v, v);
	}
}

int main(void)
{
	sys_init();
	ybs_init();
	while(1) {
		sys_update();
		ybs_update();
	}
}
