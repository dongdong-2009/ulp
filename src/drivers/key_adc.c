/* adckey.c
 * 	dusk@2010 initial version
 */

#include "key_adc.h"
#include "stm32f10x.h"
#include "time.h"

#define ADCKEY_IN GPIO_Pin_0
#define ADCKEY_NOKEY 0x0

//define adc jitter
#define ADC_JITTER	0x04

#define ADCKEY_NO	(0x0>>ADC_JITTER)
#define ADCKEY_0	(0x100>>ADC_JITTER)
#define ADCKEY_1	(0x200>>ADC_JITTER)
#define ADCKEY_2	(0x300>>ADC_JITTER)
#define ADCKEY_3	(0x400>>ADC_JITTER)
#define ADCKEY_4	(0x500>>ADC_JITTER)
#define ADCKEY_5	(0x600>>ADC_JITTER)

enum adckey_status_t{
 ADCKEY_RELEASE = 1,
 ADCKEY_DOWN,
 ADCKEY_PRESS
};
 
/*static variable*/
static key_t adckey_code; //this for up designer
static enum adckey_status_t adckey_sm;
static int adckey_counter;
static time_t key_jitter_timer;

/*local static function*/
static void adckey_Process(unsigned short key_code);

static void adckey_Process(unsigned short key_code)
{
	int left;
	
	switch(adckey_sm){
	case ADCKEY_RELEASE :
						if(key_code != ADCKEY_NO){
							adckey_counter ++;
							if(adckey_counter == 3){
								adckey_counter = 0; //reset adckey counter
								key_jitter_timer = time_get(10); //10ms delay
								adckey_sm = ADCKEY_DOWN;
							}
						}
						break;
	case ADCKEY_DOWN :
						left = time_left(key_jitter_timer);
						if(left < 0){
							if(key_code != ADCKEY_NO)
								adckey_sm = ADCKEY_PRESS;
							else
								adckey_sm = ADCKEY_RELEASE;
						}
						break;
	case ADCKEY_PRESS :
						if(key_code != ADCKEY_NO){
							switch(key_code){
							case ADCKEY_0:
										adckey_code.data = 0;
										adckey_code.flag_nokey = 0;
										break;
							case ADCKEY_1:
										adckey_code.data = 1;
										adckey_code.flag_nokey = 0;
										break;
							case ADCKEY_2:
										adckey_code.data = 2;
										adckey_code.flag_nokey = 0;
										break;
							case ADCKEY_3:
										adckey_code.data = 3;
										adckey_code.flag_nokey = 0;
										break;
							case ADCKEY_4:
										adckey_code.data = 4;
										adckey_code.flag_nokey = 0;
										break;
							case ADCKEY_5: 
										adckey_code.data = 5;
										adckey_code.flag_nokey = 0;
										break;
							default :	break;	
							}
						}
						else{
							adckey_counter ++;
							if(adckey_counter == 3){
								adckey_counter = 0;
								adckey_sm = ADCKEY_RELEASE;
							}
						}
						break;
	default :			break;
	}
}

/*
 *@initialize timer1 ch-1 to capture clock
 */
int adckey_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	
	/* Configure PC.04 (ADC Channel14) as analog input*/
	GPIO_InitStructure.GPIO_Pin = ADCKEY_IN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* ADC1 regular channel_0 configuration */ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */   
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1));	
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));

	adckey_code.flag_nokey = 1; //init no key down
	adckey_counter = 0; //reset adckey counter
	
	/*start sample*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	
	return 0;
}

void adckey_Update(void)
{
	unsigned short a;

	//read conversion status,whether end of conversion
	if(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)){
		a = ADC_GetConversionValue(ADC1);
		a = a >> ADC_JITTER; //remove jitter
		
		adckey_Process(a);
	}
}
key_t adckey_GetKey(void)
{
	return adckey_code;
}