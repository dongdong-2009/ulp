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

#define mA2d(mA) ((mA) * (20 * 4096) / 2500) //max4172, 2K => V = I * 20
#define d2mA(d) ((d) * 2500 / (20 * 4096))
#define d2mv(d) (((d) * (5 * 2500)) >> 12) //adc:  d = mv * 0.2(3KOhm / 3K + 12K) * 4096 / 2500
#define mv2d(mv) (((mv) << 12) / (5 * 2500)) //dac:  mv = d / 4096 * 2500(Vref) * 5(OPA551 gain)

static const mbi5025_t ict_mbi5025 = {
		.bus = &spi2,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PD8,
		.oe_pin = SPI_CS_PD9,
};

static int ict_vexp_1, ict_vset_1, ict_vget_1; //unit: mV
static int ict_vexp_2, ict_vset_2, ict_vget_2; //unit: mV

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

	// DAC config
	DAC_StructInit(&DAC_InitStructure);
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_1, ENABLE);

	DAC_StructInit(&DAC_InitStructure);
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	DAC_Cmd(DAC_Channel_2, ENABLE);

	/* ADC1 config, Power Ouput Voltage sampling,
	V1 = ADC1_CH2 = PA2 = ADC123_IN2
	V2 = ADC2_CH7 = PA7 = ADC12_IN7
	*/
	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 2);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_7, 2, ADC_SampleTime_71Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);

	/* ADC2 config, current sampling & over current protection
	 * I1 = ADC1_CH1 = PA1 = ADC123_IN1
	 */
	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_1, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz

	ADC_Cmd(ADC2, ENABLE);
	ADC_ResetCalibration(ADC2);
	while (ADC_GetResetCalibrationStatus(ADC2));
	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC2));

	ADC_AnalogWatchdogThresholdsConfig(ADC2, mA2d(100), 0x000);
	ADC_AnalogWatchdogSingleChannelConfig(ADC2, ADC_Channel_1);
	ADC_AnalogWatchdogCmd(ADC2, ADC_AnalogWatchdog_SingleRegEnable);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//START
	ADC_ITConfig(ADC2, ADC_IT_AWD, ENABLE);
	ADC_SoftwareStartConvCmd(ADC2, ENABLE);

	/* ADC3 config, current sampling & over current protection
	 * I2 = ADC2_CH6 = PA6 = ADC12_IN6
	 */
	ADC_DeInit(ADC3);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC3, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC3, ADC_Channel_6, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz

	ADC_Cmd(ADC3, ENABLE);
	ADC_ResetCalibration(ADC3);
	while (ADC_GetResetCalibrationStatus(ADC3));
	ADC_StartCalibration(ADC3);
	while (ADC_GetCalibrationStatus(ADC3));
	ADC_SoftwareStartConvCmd(ADC3, ENABLE);

	ADC_AnalogWatchdogThresholdsConfig(ADC3, mA2d(100),0x000);
	ADC_AnalogWatchdogSingleChannelConfig(ADC3, ADC_Channel_6);
	ADC_AnalogWatchdogCmd(ADC3, ADC_AnalogWatchdog_SingleRegEnable);

	NVIC_InitStructure.NVIC_IRQChannel = ADC3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//START
	ADC_ITConfig(ADC3, ADC_IT_AWD, ENABLE);
	ADC_SoftwareStartConvCmd(ADC3, ENABLE);

	// TIM config
	TIM_TimeBaseStructure.TIM_Period = 100 - 1; //Fclk = 10KHz /100  = 100Hz
	TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1; //prediv to 72MHz to 10KHz
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	mbi5025_Init(&ict_mbi5025);
	mbi5025_EnableOE(&ict_mbi5025);
}

void ADC1_2_IRQHandler(void)
{
	//over current, turn off relay
	GPIO_ResetBits(GPIOD, GPIO_Pin_13);
	ADC_ClearITPendingBit(ADC2, ADC_IT_AWD);
}

void ADC3_IRQHandler(void)
{
	//over current, turn off relay
	GPIO_ResetBits(GPIOD, GPIO_Pin_12);
	ADC_ClearITPendingBit(ADC3, ADC_IT_AWD);
}

void TIM2_IRQHandler(void)
{
	int d, mv;

	//handle power 1
	d = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
	ict_vget_1 = d2mv(d);
	mv = ict_vget_1 - ict_vexp_1;
	ict_vout_1 -= mv;
	d = mv2d(ict_vout_1);
	DAC_SetChannel1Data(DAC_Align_12b_R, d);

	//handle power 2
	d = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_2);
	ict_vget_2 = d2mv(d);
	mv = ict_vget_2 - ict_vexp_2;
	ict_vout_2 -= mv;
	d = mv2d(ict_vout_2);
	DAC_SetChannel2Data(DAC_Align_12b_R, d);

	ADC_SoftwareStartInjectedConvCmd(ADC1, ENABLE);
	TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
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
	int mv, temp = 0;
	int i;

	const char * usage = { \
		" usage:\n" \
		" ict relay xx xx xx ..., write value to set relay \n" \
		" ict v1 5123, set power_1 value 0~10000mv \n" \
		" ict v2 5123, set power_2 value 0~10000mv \n" \
	};

	if (argc > 2 && !strcmp(argv[1], "relay")) {
		for (i = 0; i < (argc - 2); i++) {
			sscanf(argv[2+i], "%x", &temp);
			mbi5025_WriteByte(&ict_mbi5025, temp);
		}
		spi_cs_set(ict_mbi5025.load_pin, 1);
		spi_cs_set(ict_mbi5025.load_pin, 0);
		printf("##0##");
		return 0;
	}

	if(argc == 3 && !strcmp(argv[1], "v1")) {
		mv = argv[2];
		if(mv >= 0 && mv <= 10000) {
			ict_vexp_1 = mv;
			printf("##0##");
			return 0;
		}
	}

	if(argc == 3 && !strcmp(argv[1], "v2")) {
		mv = argv[2];
		if(mv >= 0 && mv <= 10000) {
			ict_vexp_2 = mv;
			printf("##0##");
			return 0;
		}
	}

	printf(usage);
	return 0;
}
const cmd_t cmd_ict = {"ict", cmd_ict_func, "ict cmd"};
DECLARE_SHELL_CMD(cmd_ict)