/*
*
*  miaofng@2014-6-6   separate driver from irc main routine
*  miaofng@2014-9-13 remove irt hw v1.x related codes
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "stm32f10x.h"
#include "can.h"
#include "bsp.h"
#include "shell/cmd.h"

static volatile int irc_vmcomp_pulsed;

/*
TRIG	PE2	OUT
VMCOMP	PE3	IN
LE_D	PC6	OUT
LE_R	PC7	IN
MBI_OE	PB10	OUT
*/
static void gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//MBI_OE PB10 DEFAULT = HIGH/DISABLE
	GPIOB->BSRR = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//DMM TRIG & VMCOMP
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//vmcomp exti input
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource3);
	EXTI_InitStruct.EXTI_Line = EXTI_Line3;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
}

/*DPS GPIO INIT
PA0		IS_GS0
PA1		IS_GS1
PA2		LV_EN
PA3		HS_VS
PE4		HS_EN
PE5		VS_EN//CHANGED FROM HV_FS
PE6		HV_VS/IRTEST
PE7		HV_EN/IRSTART

PB6/TIM4_CH1	LV_PWM
PB7/TIM4_CH2	IS_PWM
PB8/TIM4_CH3	VS_PWM
PB9/TIM4_CH4	HV_PWM

PC0/ADC123_IN10	LV_FB
PC1/ADC123_IN11	VS_FB
PC2/ADC123_IN12	HS_FB
PC3/ADC123_IN13	HV_FB
*/

static void bsp_dps_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//GPO INIT
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//AIN
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//ADC INIT, ADC1 INJECTED CH, CONT SCAN MODE
	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 4); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_13, 4, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	//TIM4 PWM INIT
	const pwm_cfg_t cfg = {.hz = 100000, .fs = 1023};
	pwm41.init(&cfg);
	pwm42.init(&cfg);
	pwm43.init(&cfg);
	pwm44.init(&cfg);
}

void bsp_gpio_set(int pin, int high)
{
	switch(pin) {
	case IS_GS0:
		if(high) GPIOA->BSRR = GPIO_Pin_0; else GPIOA->BRR = GPIO_Pin_0;
		break;
	case IS_GS1:
		if(high) GPIOA->BSRR = GPIO_Pin_1; else GPIOA->BRR = GPIO_Pin_1;
		break;
	case LV_EN:
		if(high) GPIOA->BSRR = GPIO_Pin_2; else GPIOA->BRR = GPIO_Pin_2;
		break;
	case HS_VS:
		if(high) GPIOA->BSRR = GPIO_Pin_3; else GPIOA->BRR = GPIO_Pin_3;
		break;
	case HS_EN:
		if(high) GPIOE->BSRR = GPIO_Pin_4; else GPIOE->BRR = GPIO_Pin_4;
		break;
	case HV_VS:
		if(high) GPIOE->BSRR = GPIO_Pin_6; else GPIOE->BRR = GPIO_Pin_6;
		break;
	case HV_EN:
		if(high) GPIOE->BSRR = GPIO_Pin_7; else GPIOE->BRR = GPIO_Pin_7;
		break;
	case VS_EN:
		if(high) GPIOE->BSRR = GPIO_Pin_5; else GPIOE->BRR = GPIO_Pin_5;
		break;
	default:;
	}
}

int adc_pc0_get(void)
{
	return ADC1->JDR1 << 1;
}

int adc_pc1_get(void)
{
	return ADC1->JDR2 << 1;
}

int adc_pc2_get(void)
{
	return ADC1->JDR3 << 1;
}

int adc_pc3_get(void)
{
	return ADC1->JDR4 << 1;
}

int adc_nul_get(void)
{
	return 0;
}

/*MBI_OE OUT PB10*/
void oe_set(int high) {
	if(high) {
		GPIOB->BSRR = GPIO_Pin_10;
	}
	else {
		GPIOB->BRR = GPIO_Pin_10;
	}
}

/*TRIG PE2 OUT*/
void trig_set(int high) {
	irc_vmcomp_pulsed = 0;
	if(high) {
		GPIOE->BSRR = GPIO_Pin_2;
	}
	else {
		GPIOE->BRR = GPIO_Pin_2;
	}
}

/*VMCOMP PE3 IN*/
int trig_get(void) {
	return irc_vmcomp_pulsed;
}

/*LE_txd PC7*/
void le_set(int high) {
	if(high) {
		GPIOC->BSRR = GPIO_Pin_7;
	}
	else {
		GPIOC->BRR = GPIO_Pin_7;
	}
}

/*LE_rxd	PC6*/
int le_get(void) {
	return (GPIOC->IDR & GPIO_Pin_6) ? 1 : 0;
}

void EXTI3_IRQHandler(void)
{
	irc_vmcomp_pulsed = 1;
	EXTI->PR = EXTI_Line3;
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	irc_can_handler();
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
}

void board_init(void)
{
	gpio_init();
	bsp_dps_init();
	trig_set(0);

	const can_bus_t *irc_bus = &can1;
	const can_cfg_t irc_cfg = {.baud = CAN_BAUD, .silent = 0};
	irc_bus->init(&irc_cfg);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_SetPriority(SysTick_IRQn, 0);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void board_reset(void)
{
	NVIC_SystemReset();
}
