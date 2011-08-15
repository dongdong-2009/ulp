#ifndef   _SSS_ADC_H
#define   _SSS_ADC_H
#include "stm32f10x.h"
/*typedef struct {
	int (*init)(const pwm_cfg_t *cfg);
	int (*set)(int val);
} adc_bus_t;*/
extern uint16_t  ADCValue[16];
void ADC1_Init(void);
void ADC1_DMA_Init(void);
unsigned int ADC_filter(void);
int adc_getvalue(void);
#endif

