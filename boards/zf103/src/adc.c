/* adc.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x_lib.h"
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
	//ADC_DeInit(ADC2);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	/* Enable ADC2 */
	//ADC_Cmd(ADC2, ENABLE);
	
	/* ADC1 configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 2;
	ADC_Init(ADC1, &ADC_InitStructure);
#if 0
	/* ADC2 Configuration ------------------------------------------------------*/
	ADC_StructInit(&ADC_InitStructure);	
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);
#endif

	// Start calibration of ADC1
	ADC_StartCalibration(ADC1);
	// Start calibration of ADC2
	//ADC_StartCalibration(ADC2);

	// Wait for the end of ADCs calibration 
	while (ADC_GetCalibrationStatus(ADC1));
	//& ADC_GetCalibrationStatus(ADC2));

	//ADC_ITConfig(ADC1, ADC_IT_EOC|ADC_IT_AWD|ADC_IT_JEOC, DISABLE);
	ADC_ExternalTrigConvCmd(ADC1, ENABLE);
}

int adc_getvolt(void)
{
	/*ext voltage 0~3v3*/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	return ADC_GetConversionValue(ADC1);
}

int adc_gettemp(void)
{
	/*int temp sensor, sampling time -> 17.1us*/
	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC));
	return ADC_GetConversionValue(ADC1);
}
