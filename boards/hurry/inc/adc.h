/* adc.h
 * 	miaofng@2009 initial version
 */
#ifndef __ADC_H_
#define __ADC_H_

#include "stm32f10x.h"

#define NB_CONVERSIONS	16

#undef ADC_MODE_NORMAL
#define ADC_MODE_INJECT
#undef ADC_MODE_INJECT_DUAL

#define PHASE_U_ADC_CHANNEL     ADC_Channel_4
#define PHASE_V_ADC_CHANNEL     ADC_Channel_5
#define PHASE_W_ADC_CHANNEL     ADC_Channel_14
#define TOTAL_CHANNEL			ADC_Channel_15
#define VDC_CHANNEL				ADC_Channel_11

/*unit: mv*/
#define ADC_COUNT_TO_VOLTAGE(CNT) (CNT*3300/4096)

void adc_Init(void);
void ad_Update(void);
void adc_GetCalibration(uint16_t* pPhaseAOffset,uint16_t* pPhaseBOffset,uint16_t* pPhaseCOffset);
void adc_SetChannel(ADC_TypeDef* ADCx,uint8_t ch);

int adc_GetVolt(uint8_t ch);

/*temperature sensor para V25 = 1400 mV,avg_slope = 4.478 mV/C*/
#define V25 1400
#define AVG_SLOPE 4.478
float adc_GetTemp(void);
#endif /*__LED_H_*/
