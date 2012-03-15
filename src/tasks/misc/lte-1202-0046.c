/*
 * 	peng.guo@2012 initial version
 */

#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "stm32f10x_gpio.h"
#include "sys/task.h"
#include "mbi5025.h"
#include "dac.h"
#include "shell/cmd.h"
#include "stm32f10x.h"
#include "led.h"
#include <stdlib.h>
#include "sys/sys.h"
#include <string.h>
#include <stm32f10x_adc.h>


static const mbi5025_t ict_mbi5025 = {
		.bus = &spi2,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PD8,
		.oe_pin = SPI_CS_PD9,
};

static int dac_out1 = 0, dac_out2 = 0;

void ict_Init(void)
{
	led_Init();
	led_flash(LED_GREEN);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DAC_InitTypeDef DAC_InitStructure;

	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD| RCC_APB2Periph_GPIOC| RCC_APB2Periph_GPIOB| RCC_APB2Periph_GPIOA, ENABLE);

	// IO config
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// ADC1 config
	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 2);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_6, 2, ADC_SampleTime_71Cycles5);
	ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);

	// ADC2 config
	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_7, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz
	ADC_Cmd(ADC2, ENABLE);

	// ADC3 config
	ADC_DeInit(ADC3);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC3, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC3, ADC_Channel_2, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz
	ADC_Cmd(ADC3, ENABLE);

	// AWD & IRQ config
	ADC_AnalogWatchdogThresholdsConfig(ADC2, 0xCCC,0x000);
	ADC_AnalogWatchdogSingleChannelConfig(ADC2, ADC_Channel_2);
	ADC_AnalogWatchdogCmd(ADC2, ADC_AnalogWatchdog_SingleRegEnable);
	ADC_ITConfig(ADC2, ADC_IT_AWD, DISABLE);

	ADC_AnalogWatchdogThresholdsConfig(ADC3, 0xCCC,0x000);
	ADC_AnalogWatchdogSingleChannelConfig(ADC3, ADC_Channel_1);
	ADC_AnalogWatchdogCmd(ADC3, ADC_AnalogWatchdog_SingleRegEnable);
	ADC_ITConfig(ADC3, ADC_IT_AWD, DISABLE);


	// DAC cinfig
	DAC_StructInit(&DAC_InitStructure);
	DAC_Init(DAC_Channel_1,&DAC_InitStructure);

	DAC_StructInit(&DAC_InitStructure);
	DAC_Init(DAC_Channel_2,&DAC_InitStructure);

	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_Cmd(DAC_Channel_2, ENABLE);

	// TIM config
	TIM_TimeBaseStructure.TIM_Period = 36000;
	TIM_TimeBaseStructure.TIM_Prescaler = 0; /*Fclk = 36MHz*/
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM2, ENABLE);

	// NVIC config
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = ADC3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// ADC calibration
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	ADC_ResetCalibration(ADC2);
	while (ADC_GetResetCalibrationStatus(ADC2));
	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC2));

	ADC_ResetCalibration(ADC3);
	while (ADC_GetResetCalibrationStatus(ADC3));
	ADC_StartCalibration(ADC3);
	while (ADC_GetCalibrationStatus(ADC3));

	mbi5025_Init(&ict_mbi5025);
	mbi5025_EnableOE(&ict_mbi5025);
}

void ADC1_2_IRQHandler(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_13);
	ADC_ITConfig(ADC2, ADC_IT_AWD, DISABLE);
	ADC_ClearITPendingBit(ADC2, ADC_IT_AWD);
	//set relay
}

void ADC3_IRQHandler(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_12);
	ADC_ITConfig(ADC3, ADC_IT_AWD, DISABLE);
	ADC_ClearITPendingBit(ADC3, ADC_IT_AWD);
	//set relay
}

void TIM2_IRQHandler(void)
{
	u16 adc_v1, adc_v2;
	int del1, del2;
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
		TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);

	//read
	adc_v1 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
	adc_v2 = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);

	//adjustment dac1
	if(adc_v1 > (dac_out1 * 4 /5 * (0xfff) /10000))
		del1 = adc_v1 - (dac_out1 * 4 /5 * (0xfff) /10000);
	else del1 = (dac_out1 * 4 /5 * (0xfff) /10000) - adc_v1;
	//adjustment dac2
	if(adc_v2 > (dac_out2 * 4 /5 * (0xfff) /10000))
		del2 = adc_v2 - (dac_out2 * 4 /5 * (0xfff) /10000);
	else del2 = (dac_out2 * 4 /5 * (0xfff) /10000) - adc_v2;

	//output
	DAC_SetChannel1Data(DAC_Align_12b_R, (dac_out1 * 4 /5 * (0xfff) /10000 + del1));
	DAC_SetChannel1Data(DAC_Align_12b_R, (dac_out2 * 4 /5 * (0xfff) /10000 + del2));

	//enable adc
	ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);
}

void main(void)
{

	task_Init();
	ict_Init();
	while(1) {
		task_Update();
	}
}

static int cmd_ict_func(int argc, char *argv[])
{
	int temp = 0;
	int i;

	const char * usage = { \
		" usage:\n" \
		" ict relay xx xx xx ..., write value to set relay \n" \
		" ict power_1 5123, set power_1 value 0~10000mv \n" \
		" ict power_2 5123, set power_2 value 0~10000mv \n" \
	};

	if (argc > 1) {
		if (argv[1][0] == 'r') {
			for (i = 0; i < (argc - 2); i++) {
				sscanf(argv[2+i], "%x", &temp);
				mbi5025_WriteByte(&ict_mbi5025, temp);
			}
			spi_cs_set(ict_mbi5025.load_pin, 1);
			spi_cs_set(ict_mbi5025.load_pin, 0);
			printf("%d Bytes Write Successful!\n", argc-2);
		}
		if (argv[1][0] == 'p') {
			if(atoi(argv[2]) < 0 || atoi(argv[2]) > 10000)
				printf("input error ,please input a value between 0 and 10000!\n");
			else {
				if(argv[1][6] == '1') {
					dac_out1 = atoi(argv[2]);
					TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				}
				if(argv[1][6] == '2') {
					dac_out2 = atoi(argv[2]);
					TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
				}
			}
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_ict = {"ict", cmd_ict_func, "ict cmd"};
DECLARE_SHELL_CMD(cmd_ict)