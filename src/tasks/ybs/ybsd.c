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
#include "ybsd.h"
#include "ybs.h"

void ADC_IRQHandler(void)
{
	ybs_isr();
}

int ybsd_vi_init(int adcflt)
{
	ADCMDE = (1 << 7) | 3; //force adc to idle mode
	IEXCON = (3 << 6) | (5 << 1) | 1; //Iexc = 1mA * 2, 10uA diag current source = on
	ADCFLT = adcflt;

	ADC0CON = 0x00; //disable both adc first
	ADC1CON = 0x00;
	ADCCFG = 0;

	//ADC0 = ADUC_MUX0_DCH01 | PGA_512 | BIPOLAR
	ADC0CON = (1 << 15) | (1 << 11) | 9;
	//ADC1 = ADUC_MUX1_DCH23 | PGA_8 | BIPOLAR | FULL BUF ON
	//ADC1CON = (1 << 15) | 3;

	/*zcal*/
	ADCMDE = (1 << 7) | 0x04; //self offset calibration
	time_t deadline = time_get(500);
	while((ADCSTA & 0x01) == 0) {
		if(time_left(deadline) < 0) {
			ybs_error(YBS_E_ADC_ZCAL);
			break;
		}
	}

	ADCMDE = (1 << 7) | 0x01;
	ADCMSKI = 1 << 0;
	return 0;
}

int ybsd_vo_init(void)
{
	DACCON = (1 << 4) | (1 << 3) ; //16bit mode, 0~1V2
	ADCCFG |= 1 << 7; //enable GND_SW
	return 0;
}

int ybsd_set_vo(int data)
{
	data = (data > 65535) ? 65535 : data;
	data = (data < 0) ? 0 : data;
	DACDAT = data << 12;
	return 0;
}

//p2.0
void ybsd_rb_init(void)
{
	GP2CON &= ~0x03;
	GP2DAT &= ~ (1 << 24);
	GP2PAR &= ~ (1 << 0); //enable 2mA pull-up
}

/*pressed return 0*/
int ybsd_rb_get(void)
{
	return GP2DAT & 0x01;
}
