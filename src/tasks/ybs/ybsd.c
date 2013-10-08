/*
*
* 1, ybs power-up, if calibration data is incorrect, flash error led
* 2, ybs do continously adc and update analog output
* 3, according to 485 command, do corresponding response
*
* 1, ybs command interface can only change 
*
*/
#include "ulp/sys.h"
#include "aduc706x.h"

#define CONFIG_ADUC_50HZ_CHOP 1

int ybsd_vi_init(void)
{
	ADCMDE = (1 << 7) | 3; //force adc to idle mode
	IEXCON = (3 << 6) | (5 << 1) | 1; //Iexc = 1mA * 2, 10uA diag current source = on
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

	//ADC0 = ADUC_MUX0_DCH01 | PGA_512 | BIPOLAR
	ADC0CON = (1 << 15) | (1 << 11) | 9;
	//ADC1 = ADUC_MUX1_DCH23 | PGA_8 | BIPOLAR | FULL BUF ON
	ADC1CON = (1 << 15) | 3;

	/*zcal*/
	ADCMDE = (1 << 7) | 0x04; //self offset calibration
	while((ADCSTA & 0x01) == 0);

	ADCMDE = (1 << 7) | 0x01;
	return 0;
}

int ybsd_vo_init(int vref_mv)
{
	//p2.0 voltage<1>/resistor<0> mode switch
	GP2DAT |= 1 << (24 + 0); //DIR = OUT
	DACCON = (1 << 4); //12bit mode, 0~1V2
	//DACDAT = DMM_UV_TO_D(1000*1000); //default 1V
}

int ybsd_set_vo(int mv)
{
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
}