/* adc.h
 * 	miaofng@2009 initial version
 */
#ifndef __ADC_H_
#define __ADC_H_

#include "stm32f10x.h"

#undef ADC_MODE_NORMAL
#define ADC_MODE_INJECT
#define ADC_MODE_INJECT_DUAL

void adc_init(void);
/*zf32 board: PA1/ADC_IN1, JP1 R7 POT_WHEEL 0~3v3*/
/*unit: mv*/
#define ADC_COUNT_TO_VOLTAGE(CNT) (CNT*3300/4096)
int adc_getvolt(void); 

/*temperature sensor para*/
/*V25 = 1400 mV,avg_slope = 4.478 mV/C*/
#define V25 1400
#define AVG_SLOPE 4.478
float adc_gettemp(void);
//void ad_update(void);
#endif /*__LED_H_*/
