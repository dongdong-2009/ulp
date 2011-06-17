/* this project is used to emulate vehicle spark waveform, then measure the peak current and voltage,
according to these two value, dut igbt performance cauld be identified.
there are 4 hardware modules in total:
1, pmm - period measure module --- HW TIMER
2, mos - spark discharge control module, core of spark waveform control --- HW TIMER
3, ipm - spark current measure module --- loop
4, vpm - spark voltage measure module --- DMA
*/
#include "config.h"
#include "debug.h"
#include "time.h"
#include "nvm.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "stm32f10x.h"

#define CONFIG_USE_AD9203	0

/*current measurement start time point at 38mS*/
#define ipm_start_us_def	38000
#define mos_delay_us_def	2.5
#define mos_close_us_def	50

static unsigned short vpm_data[256];
static unsigned short mos_delay_clks __nvm;
static unsigned short mos_close_clks __nvm;
static unsigned short ipm_start_us __nvm;

/*burn system state machine*/
static enum {
	BURN_INIT,
	BURN_IDLE, //1st pulse detected, idle period, normal 38mS
	BURN_MEAS, //measurement period, normal 2mS
} burn_state;

/*burn statistics data*/
static struct {
	/*spark period, unit: uS, range: 0 ~ 40,000 uS*/
	int tp_avg;
	int tp_min;
	int tp_max;

	/*unit: V, range: 0 ~ 500V*/
	int vp_avg;
	int vp_min;
	int vp_max;

	/*unit: mA, range: 0 ~ 10,000mA*/
	int ip_avg;
	int ip_min;
	int ip_max;
} burn_data;

/*mos control
	TIM2 CH1,  worked in slave mode,  drive external high voltage mos switch on and off
	after TIM2 CH2 spark signal mos_on_delay clks
*/
void mos_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//PA1 TRIG IN(high effective)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA0 MOS DRV Output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = mos_delay_clks + mos_close_clks;
	TIM_TimeBaseStructure.TIM_Prescaler = 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* TIM2 PWM2 Mode configuration: Channel1 */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = mos_delay_clks;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	/* TIM2 configuration in Input Capture Mode */
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	/* One Pulse Mode selection */
	TIM_SelectOnePulseMode(TIM2, TIM_OPMode_Single);

	/* Input Trigger selection */
	TIM_SelectInputTrigger(TIM2, TIM_TS_TI2FP2);

	/* Slave Mode selection: Trigger Mode */
	TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Trigger);
	TIM_Cmd(TIM2, ENABLE);
}

void vpm_Init(void)
{
#if CONFIG_USE_AD9203 == 1
/* VP, optional ad9203 40msps 10bit parallel pipeline ADC
	D0 ~ D9	<=>	PC6~15
	CLK	<=>	TIM3 CH1(PA6), TIM3 CH2(PA7) is external trigger input of spark signal
*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	//D0 ~ D9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//PA7 trig input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA6 trig output of AD9203 clock
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#else /*CONFIG_USE_AD9203 == 1*/
/*
	VP, ADC1 regular channel 4(PA4), DMA mode capture, hard triggered by EXTI11(PA11),
		continuously sample, until dma buf overflow, poll dma result
*/
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef  DMA_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//PA11 TRIG IN(high effective)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA4 analog voltage input, ADC12_IN4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_Ext_IT11_TIM8_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_1Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	//setup dma
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(DMA1_Channel1);
	DMA_Cmd(DMA1_Channel1, DISABLE);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&ADC1 -> DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) vpm_data;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = sizeof(vpm_data);
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);

	ADC_DMACmd(ADC1, ENABLE);
#endif
}

void vpm_Update(void)
{
}

/*
	IP, ADC2 regular channel 5(PA5), PIO mode capture(soft triggered by TIM4 CH1,
	then software convert and covert ... get a maximum current value)
*/
void ipm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	//PA5 IP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_5, 1, ADC_SampleTime_1Cycles5);
	ADC_Cmd(ADC2, ENABLE);

	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC2));
}

void ipm_Update(void)
{

}

/* period measure module: F = 1MHz
	TIM4 CH2(PB7) is used for ignition pulse period capture
*/
void pmm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//TRIG IN(high effective)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//TRIG Output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0xffff;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; /*Fclk = 1MHz*/
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	/* TIM4 CH1 = PWM2 Mode*/
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0xffff;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);

	/* TIM4 CH2 = Input Capture Mode */
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x15;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);

	TIM_Cmd(TIM4, ENABLE);
	TIM_UpdateRequestConfig(TIM4, TIM_UpdateSource_Global); //Setting UG won't lead to an UEV

	//poll mode check
	TIM_ClearFlag(TIM4, TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_Update);
}

static int pmm_t1 = 1; //random value
static int pmm_t2 = 0; //random value

void pmm_Update(void)
{
	int f, v, det;

	f = TIM4->SR;
	TIM4->SR = 0;

	if(f & TIM_FLAG_Update) { //time over 65.535 mS
		if(burn_state != BURN_INIT) {
			if(f & TIM_FLAG_CC2) {
				v = TIM4->CCR2;
				pmm_t2 += (v > 10000) ? 0 : 0x10000; //over 10mS? update after capture
			}
			else pmm_t2 += 0x10000;

			//for ipm trig
			det = pmm_t2 - pmm_t1;
			det = ipm_start_us - det;
			if((det > 0) && (det < 0xffff)) { //in current update period, enable trig
				TIM4->CCR1 = det;
			}

		}
	}

	if(f & TIM_FLAG_CC2) {
		v = TIM4->CCR2;
		if(burn_state == BURN_INIT) {
			pmm_t1 = v;
			pmm_t2 = 0;
			burn_state = BURN_IDLE;
		}
		else {
			pmm_t2 += v;
			det = pmm_t2 - pmm_t1;
			pmm_t1 = v;
			pmm_t2 = 0;

			//det process
			burn_data.tp_max = (burn_data.tp_max > det) ? burn_data.tp_max : det;
			burn_data.tp_min = (burn_data.tp_max < det) ? burn_data.tp_min : det;
			burn_data.tp_avg = (burn_data.tp_avg + det) >> 1;

			//debug
			if(0) {
				printf("tp = %03d.%03dmS\n", det/1000, det%1000);
			}

			burn_state = BURN_IDLE;
		}
	}

	if(f & TIM_FLAG_CC1) { //igbt charge is about to start ....
		if(burn_state == BURN_IDLE) {
			burn_state = BURN_MEAS;
		}
	}
}

/*communication with host*/
void com_Init(void)
{
}

void com_Update(void)
{
}

void burn_Init()
{
	burn_state = BURN_INIT;
	memset(burn_data, 0, sizeof(burn_data));

	if(mos_delay_clks == 0xffff) { //set default value
		mos_delay_clks  = (unsigned short) (mos_delay_us_def * 36); //unit: 1/36 us
		mos_close_clks  = (unsigned short) (mos_close_us_def * 36); //unit: 1/36 us
		ipm_start_us = ipm_start_us_def;
		nvm_save();
	}

	pmm_Init();
	mos_Init();
	//vpm_Init();
	ipm_Init();
	//com_Init();
}

void burn_Update()
{
	pmm_Update();
	//vpm_Update();
	ipm_Update();
	//com_Update();
}

void main(void)
{
	task_Init();
	burn_Init();
	while(1) {
		task_Update();
		burn_Update();
	}
}
