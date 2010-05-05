/* adckey.c
 * 	dusk@2010 initial version
 *	miaofng@2010 state machine change
 */

#include "key_adc.h"
#include "stm32f10x.h"
#include "time.h"

#define ADCKEY_PIN GPIO_Pin_0
#define ADCKEY_JITTER (1 << 7) /* (0x40/4096)*3300 = +/-51mv */
#define ADCKEY_UPDATE_MS (10)
#define ADCKEY_DETECT_CNT (5) /*detect time = ADCKEY_DETECT_CNT * ADCKEY_UPDATE_MS*/

enum {
 ADCKEY_IDLE = 0, /*key is released*/
 ADCKEY_PRESS, /*key is pressed*/
};

/*static variable*/
static key_t adckey; //this for up designer
static short adckey_sm;
static short adckey_counter;
static time_t adckey_timer;

bool adckey_Process(bool pressed)
{
	bool update = 0;

	switch(adckey_sm){
	case ADCKEY_IDLE:
		if(pressed){
			adckey_counter ++;
			if(adckey_counter >= ADCKEY_DETECT_CNT) {
				adckey_counter = 0;
				adckey_sm = ADCKEY_PRESS;
				update = 1;
			}
		}
		else
			adckey_counter = 0;
		break;
	case ADCKEY_PRESS:
		if(!pressed){
			adckey_counter ++;
			if(adckey_counter >= ADCKEY_DETECT_CNT){
				adckey_counter = 0;
				adckey_sm = ADCKEY_IDLE;
				update = 1;
			}
		}
		else
			adckey_counter = 0;
		break;
	default:
		adckey_sm = ADCKEY_IDLE;
		break;
	}
	
	return update;
}

int adckey_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/

	/* Configure PC.04 (ADC Channel14) as analog input*/
	GPIO_InitStructure.GPIO_Pin = ADCKEY_PIN;
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

	/*start sample*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	/*init static variables*/
	adckey.value = 0;
	adckey.flag_nokey = 1;
	adckey_counter = 0;
	adckey_sm = ADCKEY_IDLE;
	adckey_timer = time_get(ADCKEY_UPDATE_MS);
	return 0;
}

void adckey_Update(void)
{
	int value;
	bool update;

	if(time_left(adckey_timer) < 0)
		return;

	adckey_timer = time_get(ADCKEY_UPDATE_MS);
	if(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC))
		return;

	value = ADC_GetConversionValue(ADC1);
	update = adckey_Process(value > ADCKEY_JITTER);
	if(update) {
		value += ADCKEY_JITTER >> 1;
		value &= (ADCKEY_JITTER - 1);
		if(value & 0xff) { //error situation
			value = 0;
		}

		value >>= 8;
		adckey.value = 0;
		if(value == 0)
			adckey.flag_nokey = 1;
		else
			adckey.code = value - 1;
	}

	//start adc convert again ...

}

key_t adckey_GetKey(void)
{
	adckey_Update();
	return adckey;
}
