/*
 * junjun@2011 initial version
 */

#include <stdio.h>
#include "config.h"
#include "lib/ybs.h"
#include "sys/task.h"
#include "stm32f10x.h"
#include "ulp/device.h"
#include "ulp/dac.h"

#define FZ  100000

static int adc_value;
static int dac_value;
static int fd;
static char buf[6];

int adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//PA1 analog voltage input, ADC12_IN1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  //Single MODE
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5); //9Mhz/(239.5 + 12.5) = 35.7Khz
	//ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, ENABLE);

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	return 0;
}

int timer_init(int sample_fz)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = sample_fz;
	TIM_TimeBaseStructure.TIM_Prescaler = 16 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	//TIM_ARRPreloadConfig(TIM4, ENABLE);

	TIM_Cmd(TIM4, ENABLE);

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	return 0;
}


void TIM4_IRQHandler(void)
{
	//printf("adc value is %d\n",ADC_GetConversionValue(ADC1));
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, DISABLE);
	adc_value = ADC_GetConversionValue(ADC1);
	printf("\nADC convert result:%d\n",adc_value);
	printf("DAC input value   :%d\n",dac_value);
	if(adc_value < 2000){
		printf("ADC convert result:%d\n",adc_value);
		printf("DAC input value   :%d\n",dac_value);
		TIM_Cmd(TIM4, DISABLE);
	}
	dac_value += 2;
	if(dac_value > 65535)
		dac_value = 0;
	sprintf(buf,"%d",dac_value);
	dev_ioctl(fd,DAC_CHOOSE_CH,0);
	dev_write(fd,buf,0);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}



/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void main(void)
{
	task_Init();
	ybs_Init();
	adc_init();
	timer_init(FZ);
	fd = dev_open("dac", 0);
	printf("Power Conditioning - YBS\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		task_Update();
		ybs_Update();
		//gpcon_signal(SENSOR,SENPR_ON);
		//ybs_mdelay(5000);
		//gpcon_signal(SENSOR,SENPR_OFF);
		//ybs_mdelay(5000);
	}
} // main
