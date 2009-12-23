/* adc.h
 * 	miaofng@2009 initial version
 */
#ifndef __ADC_H_
#define __ADC_H_

#include "stm32f10x.h"

#undef ADC_MODE_NORMAL
#define ADC_MODE_INJECT
#undef ADC_MODE_INJECT_DUAL

#define	U_CHANNEL	ADC_Channel_4
#define V_CHANNEL	ADC_Channel_5
#define W_CHANNEL	ADC_Channel_14
#define TOTAL_CHANNEL	ADC_Channel_15

void adc_init(void);
/*zf32 board: PA1/ADC_IN1, JP1 R7 POT_WHEEL 0~3v3*/
/*unit: mv*/
#define ADC_COUNT_TO_VOLTAGE(CNT) (CNT*3300/4096)
int adc_getvolt1(void); 

int adc_getvolt(uint8_t ch);

/*temperature sensor para*/
/*V25 = 1400 mV,avg_slope = 4.478 mV/C*/
#define V25 1400
#define AVG_SLOPE 4.478
float adc_gettemp(void);
//void ad_update(void);
#endif /*__LED_H_*/
