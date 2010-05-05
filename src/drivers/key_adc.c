/* adckey.c
 * 	dusk@2010 initial version
 */

#include "key_adc.h"
#include "stm32f10x.h"
#include "time.h"

#define ADCKEY_IN GPIO_Pin_0

#define ADCKEY_NO	0xff
#define ADCKEY_0	0x100
#define ADCKEY_1	0x200
#define ADCKEY_2	0x300
#define ADCKEY_3	0x400
#define ADCKEY_4	0x500
#define ADCKEY_5	0x600

/* (0x40/4096)*3300 = 51mv */
#define ADCKEY_JITTER 0x40 

/*
 *ADCKEY_IDLE:this state is no key event
 *ADCKEY_DOWN:the key is just pressing down,is temp state
 *ADCKEY_PRESS:the key has pressed down
 *ADCKEY_UP:the key is just pressing up,is temp state
 */
enum {
 ADCKEY_IDLE = 0,
 ADCKEY_DOWN,
 ADCKEY_PRESS,
 ADCKEY_UP
};
 
/*static variable*/
static key_t adckey; //this for up designer
static int adckey_sm;
static int adckey_counter;
static time_t key_jitter_timer;

/*local static function*/
static void adckey_Process(unsigned char key_code);

static void adckey_Process(unsigned char key_code)
{
	int left;
	
	switch(adckey_sm){
	case ADCKEY_IDLE:
		if(key_code != ADCKEY_NO){
			adckey_counter ++;
			if(adckey_counter > 2){
				adckey_counter = 0; //reset adckey counter
				key_jitter_timer = time_get(5); //5ms delay
				adckey_sm = ADCKEY_DOWN;
			}
		}
		else{
			adckey.flag_nokey = 1;
		}
		break;
	case ADCKEY_DOWN:
		left = time_left(key_jitter_timer);
		if(left < 0){
			if(key_code != ADCKEY_NO)
				adckey_sm = ADCKEY_PRESS;
			else
				adckey_sm = ADCKEY_IDLE;
		}
		break;
	case ADCKEY_PRESS:
		if(key_code != ADCKEY_NO){
			adckey.data = key_code;
			adckey.flag_nokey = 1;
		}
		else{
			adckey_counter ++;
			if(adckey_counter > 2){
				adckey_counter = 0;
				key_jitter_timer = time_get(2); //2ms delay
				adckey_sm = ADCKEY_UP;
			}
		}
		break;
	case ADCKEY_UP:
		left = time_left(key_jitter_timer);
		if(left < 0){	
			if(key_code != ADCKEY_NO)
				adckey_sm = ADCKEY_PRESS;
			else
				adckey_sm = ADCKEY_IDLE;
		}
		break;
	default:
		break;
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

	/*init related local variables*/
	adckey.flag_nokey = 1; //init no key down
	adckey_counter = 0; //reset adckey counter
	adckey_sm = ADCKEY_IDLE;
	
	/*start sample*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	
	return 0;
}

void adckey_Update(void)
{
	unsigned short a;
	unsigned char b;

	//read conversion status,whether end of conversion
	if(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)){
		a = ADC_GetConversionValue(ADC1);
		
		if(a < (ADCKEY_0 - ADCKEY_JITTER) && a > (ADCKEY_0 - ADCKEY_JITTER))
			b = 0;
		else if(a < (ADCKEY_1 - ADCKEY_JITTER) && a > (ADCKEY_1 - ADCKEY_JITTER))
			b = 1;
		else if(a < (ADCKEY_2 - ADCKEY_JITTER) && a > (ADCKEY_2 - ADCKEY_JITTER))
			b = 2;
		else if(a < (ADCKEY_3 - ADCKEY_JITTER) && a > (ADCKEY_3 - ADCKEY_JITTER))
			b = 3;
		else if(a < (ADCKEY_4 - ADCKEY_JITTER) && a > (ADCKEY_4 - ADCKEY_JITTER))
			b = 4;
		else if(a < (ADCKEY_5 - ADCKEY_JITTER) && a > (ADCKEY_5 - ADCKEY_JITTER))
			b = 5;
		else
			b = ADCKEY_NO;
		
		adckey_Process(b);
	}
}

key_t adckey_GetKey(void)
{
	return adckey;
}