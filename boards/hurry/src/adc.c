/* adc.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "adc.h"

/*zf32 board: PA1/ADC_IN1, JP1 R7 POT_WHEEL 0~3v3*/
void adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	/* Configure PA.1 as Analog Input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ADC1 registers reset ----------------------------------------------------*/
	ADC_DeInit(ADC1);
	/* ADC2 registers reset ----------------------------------------------------*/
	ADC_DeInit(ADC2);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	/* Enable ADC2 */
	ADC_Cmd(ADC2, ENABLE);
	
	/* ADC1 configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 2;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* ADC2 Configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);	
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);

	// Start calibration of ADC1
	ADC_StartCalibration(ADC1);
	// Start calibration of ADC2
	ADC_StartCalibration(ADC2);

	// Wait for the end of ADCs calibration 
	while (ADC_GetCalibrationStatus(ADC1) & ADC_GetCalibrationStatus(ADC2));

	//ADC_ITConfig(ADC1, ADC_IT_EOC|ADC_IT_AWD|ADC_IT_JEOC, DISABLE);
}

/*ext voltage 0~3v3*/
int adc_getvolt1(void)
{
	int value;
#ifdef ADC_MODE_NORMAL
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	value = ADC_GetConversionValue(ADC1);
#endif

#ifdef ADC_MODE_INJECT
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
	ADC_InjectedSequencerLengthConfig(ADC1, 1);

	#ifdef ADC_MODE_INJECT_DUAL
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC2, ENABLE);
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
	ADC_InjectedSequencerLengthConfig(ADC2, 1);

	/*note: ADC mode is already ADC_Mode_InjecSimult, refer ADC_Init(..)*/
	#endif

	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
	ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC));
	value = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
	#ifdef ADC_MODE_INJECT_DUAL
	value = ADC_GetInjectedConversionValue(ADC2, ADC_InjectedChannel_1);
	#endif
#endif

	value &= 0x0fff;
	return ADC_COUNT_TO_VOLTAGE(value);
}


/*ext voltage 0~3v3*/
int adc_getvolt(uint8_t ch)
{
	int value;
	
#ifdef ADC_MODE_NORMAL
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	value = ADC_GetConversionValue(ADC1);
#endif

#ifdef ADC_MODE_INJECT
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
	ADC_InjectedChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
	ADC_InjectedSequencerLengthConfig(ADC1, 1);

	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
	ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC));
	value = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
#endif

	value &= 0x0fff;
	return ADC_COUNT_TO_VOLTAGE(value);
}

float adc_gettemp(void)
{
	int temp1 = 0;
        float temp2 = 0;
	/*int temp sensor, sampling time -> 17.1us*/
	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	temp1 = ADC_GetConversionValue(ADC1);
        
	temp1 &= 0x0fff;
        temp2 =(V25- (float)temp1*3300/4096)/(float)AVG_SLOPE + 25.0; 
	return temp2;
}
