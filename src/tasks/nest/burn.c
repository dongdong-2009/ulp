/* this project is used to emulate vehicle spark waveform, then measure the peak current and voltage,
according to these two value, dut igbt performance cauld be identified.
there are 4 hardware modules in total:
1, pmm - period measure module & timing unit --- HW TIMER - TIM4
2, mos - spark discharge control module, core of spark waveform control --- HW TIMER - TIM2
3, ipm - spark current measure module --- poll
4, vpm - spark voltage measure module --- DMA
*/
#include "config.h"
#include "debug.h"
#include "time.h"
#include "nvm.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "stm32f10x.h"
#include <string.h>

#define CONFIG_USE_AD9203	1

#define T			20 //spark period, unit: ms
#define ipm_ms			4 //norminal ipm measurement time
#define vpm_ms			1 //norminal vpm measurement time
#define mos_delay_us_def	0.8 //range: 1/36 ~ 65535/36 uS
#define mos_close_us_def	1000 //range: 1/36 ~ 65535/36 uS

//rough waveform data, captured by vpm_Update & ipm_Update
#define VPM_FIFO_N	256
#define IPM_FIFO_N	256
static unsigned short vpm_data[VPM_FIFO_N];
static unsigned short ipm_data[IPM_FIFO_N];
static short vpm_data_n = 0;
static short ipm_data_n = 0;

static unsigned short mos_delay_clks __nvm;
static unsigned short mos_close_clks __nvm;
static unsigned short burn_ms __nvm;
static time_t burn_timer;
static time_t burn_tick; //for debug purpose

/*burn system state machine*/
static enum {
	BURN_INIT,
	BURN_IDLE, //1st pulse detected, idle duration, normal 36mS
	BURN_MEAI, //current measurement duration, normal 4mS
	BURN_MEAV, //voltage measurement duration, normal < 1mS
} burn_state;

enum {
	START,
	UPDATE,
	STOP,
};

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
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
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
/* VP, optional ad9203 40msps 10bit parallel pipeline ADC
	D0 ~ D9	<=>	PC6~15
	CLK	<=>	TIM3 CH1(PA6), TIM3 CH2(PA7) is external trigger input of spark signal
*/
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	DMA_InitTypeDef  DMA_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//D0 ~ D9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//PA7 trig input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA6 trig output of AD9203 clock
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period =  3; //Fclk = 72Mhz / 4 = 18Mhz
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* TIM2 PWM2 Mode configuration: Channel1 */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 2; //duty factor = 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);

	/* TIM2 configuration in Input Capture Mode */
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	/* One Pulse Mode selection */
	TIM_SelectOnePulseMode(TIM3, TIM_OPMode_Repetitive);

	/* Input Trigger selection */
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI2FP2);

	/* Slave Mode selection: Trigger Mode */
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Gated);

	//setup dma
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(DMA1_Channel6);
	DMA_Cmd(DMA1_Channel6, DISABLE);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&GPIOC -> IDR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) vpm_data;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = VPM_FIFO_N;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel6, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel6, ENABLE);

	TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
        TIM_Cmd(TIM3, ENABLE);
}

/*v1.1 circuit bug, d0 ~ d9 exchange connection*/
unsigned short vpm_correct(unsigned short x)
{
	unsigned short y = 0;
	y |= (x & 0x8000) >> 9; //ad9203 bit 0
	y |= (x & 0x4000) >> 7; //ad9203 bit 1
	y |= (x & 0x2000) >> 5; //ad9203 bit 2
	y |= (x & 0x1000) >> 3; //ad9203 bit 3
	y |= (x & 0x0800) >> 1; //ad9203 bit 4
	y |= (x & 0x0400) << 1; //ad9203 bit 5
	y |= (x & 0x0200) << 3; //ad9203 bit 6
	y |= (x & 0x0100) << 5; //ad9203 bit 7
	y |= (x & 0x0080) << 7; //ad9203 bit 8
	y |= (x & 0x0040) << 9; //ad9203 bit 9
	return y;
}

void vpm_Update(int ops)
{
	int n;

	switch(ops) {
	case START:
		//vpm_data_n = VPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel6);
		break;
	case UPDATE:
		break;
	case STOP:
		n = VPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel6);
		if(0) {
			int i = (n > vpm_data_n) ? (n - vpm_data_n) : (VPM_FIFO_N - vpm_data_n + n);
			printf("vpm_data[%d] = \n", i);
			if(n > vpm_data_n) { //normal
				for(i = vpm_data_n; i < n; i ++)
				printf("%d ", vpm_data[i]);
			}
			else {
				for(i = vpm_data_n; i < VPM_FIFO_N; i ++)
					printf("%d ", vpm_correct(vpm_data[i]));
				for(i = 0; i < n; i ++)
					printf("%d ", vpm_correct(vpm_data[i]));
			}
			printf("\n");
		}
		vpm_data_n = n;
		break;

	default:
		break;
	}
}

/*
	IP, ADC1 regular channel 5(PA5), DMA mode capture, soft trig
		continuously sample, until hardware trig signal reach
*/
void ipm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef  DMA_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	//PA11 TRIG IN(high effective)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Select the EXTI Line11 the GPIO pin source */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource11);
	/* EXTI line11 configuration -----------------------------------------------*/
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_Line = EXTI_Line11;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	//PA4 analog voltage input, ADC12_IN4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//ADC_ExternalTrigConv_Ext_IT11_TIM8_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_239Cycles5); //9Mhz/(239.5 + 12.5) = 35.7Khz
	//ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, DISABLE);

	//setup dma
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(DMA1_Channel1);
	DMA_Cmd(DMA1_Channel1, DISABLE);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&ADC1 -> DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) ipm_data;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = VPM_FIFO_N;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);

	ADC_DMACmd(ADC1, ENABLE);
}

void ipm_Update(int ops)
{
	int n;

	switch(ops) {
	case START:
		ADC_Cmd(ADC1, ENABLE);
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		break;
	case UPDATE:
		break;
	case STOP:
		ADC_Cmd(ADC1, DISABLE);
		n = IPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel1);
		if(1) {
			int i = (n > ipm_data_n) ? (n - ipm_data_n) : (IPM_FIFO_N - ipm_data_n + n);
			printf("ipm_data[%d] = \n", i);
			if(n > ipm_data_n) { //normal
				for(i = ipm_data_n; i < n; i ++)
				printf("%d ", ipm_data[i]);
			}
			else {
				for(i = ipm_data_n; i < IPM_FIFO_N; i ++)
					printf("%d ", ipm_data[i]);
				for(i = 0; i < n; i ++)
					printf("%d ", ipm_data[i]);
			}
			printf("\n");
		}
		ipm_data_n = n;
		break;
	default:
		break;
	}
}

#define pmm_T	0x10000 //pmm overflow time, unit: uS, <= 0x10000

/* period measure module: F = 1MHz
	TIM4 CH2(PB7) is used for ignition pulse period capture
*/
void pmm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	//TIM_OCInitTypeDef TIM_OCInitStructure;

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
	TIM_TimeBaseStructure.TIM_Period = pmm_T - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; /*Fclk = 1MHz*/
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

#if 0
	/* TIM4 CH1 = PWM2 Mode*/
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0xffff;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);
#endif

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
	TIM_ClearFlag(TIM4, TIM_FLAG_CC2 | TIM_FLAG_Update);
}

static int pmm_t1;
static int pmm_t2;

//para ignore is used to avoid error trig signal in some duration
int pmm_Update(int ops, int ignore)
{
	int f, v, det, t1, t2;

	f = TIM4->SR;
	TIM4->SR = ~f;

	t1 = 0; //count to current period, these two paras are added to solve issue:  "bldc# 142543    -45.-522mS"
	t2 = 0; //count to next period
	if(f & TIM_FLAG_CC2)
		v = TIM4->CCR2;

	if(f & TIM_FLAG_Update) { //time over 65.535 mS
		t2 = pmm_T; //update after capture
		if(f & TIM_FLAG_CC2) {
			if(v < 10000) { //update before capture
				t1 = pmm_T;
				t2 = 0;
			}
		}
	}

	if(f & TIM_FLAG_CC2) {
		if((ops == UPDATE) && ignore) {
			//printf("_");
			pmm_t2 += t1 + t2;
			return 0;
		}

		if(ops == START) {
			pmm_t1 = v;
			pmm_t2 = t2;
			burn_tick = time_get(0);
		}
		else {
			pmm_t2 += v + t1;
			det = pmm_t2 - pmm_t1;
			pmm_t1 = v;
			pmm_t2 = t2;

			//det process
			burn_data.tp_max = (burn_data.tp_max > det) ? burn_data.tp_max : det;
			burn_data.tp_min = (burn_data.tp_max < det) ? burn_data.tp_min : det;
			burn_data.tp_avg = (burn_data.tp_avg + det) >> 1;

			//debug
			if((det > 21000) || (det < 19000)) {
				printf("%d	%03d.%03dmS\n", -time_left(burn_tick), det/1000, det%1000);
			}
		}
		return 1;
	}

	pmm_t2 += t2;
	return 0;
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
	memset(&burn_data, 0, sizeof(burn_data));

	if(1){//burn_ms == 0xffff) { //set default value
		mos_delay_clks  = (unsigned short) (mos_delay_us_def * 36); //unit: 1/36 us
		mos_close_clks  = (unsigned short) (mos_close_us_def * 36); //unit: 1/36 us
		burn_ms = T;
		nvm_save();
	}

	pmm_Init();
	mos_Init();
	com_Init();
	ipm_Init();
        vpm_Init();
}

void burn_Update()
{
	int flag, loop;

	do {
		loop = 0;
		switch(burn_state) {
		case BURN_INIT:
			flag = pmm_Update(START, 1);
			if(flag) {
				burn_state = BURN_IDLE;
				burn_timer = time_get(burn_ms - ipm_ms);
			}
			break;

		case BURN_IDLE:
			pmm_Update(UPDATE, 1); //ignore err trig
			if(time_left(burn_timer) < 0) {
				burn_state = BURN_MEAI;
				burn_timer = time_get(ipm_ms * 2);
				ipm_Update(START);
				vpm_Update(START); //start vpm asap
			}
			break;

		case BURN_MEAI:
			ipm_Update(UPDATE);
			flag = pmm_Update(UPDATE, 0);
			if(flag) {
				burn_state = BURN_MEAV;
				burn_timer = time_get(vpm_ms);
				ipm_Update(STOP);
				printf("%d	trig!!!\n", -time_left(burn_tick));
				break;
			}
			if(time_left(burn_timer) < 0) { //lost trig signal at this cycle??  resync
				burn_state = BURN_INIT;
				burn_timer = 0;
				printf("%d	reset:(\n", -time_left(burn_tick));
				break;
			}
			loop = 1;
			break;

		case BURN_MEAV:
			pmm_Update(UPDATE, 1);
			vpm_Update(UPDATE);
			if(time_left(burn_timer) < 0) {
				burn_state = BURN_IDLE;
				burn_timer = time_get(burn_ms - vpm_ms - ipm_ms);
				vpm_Update(STOP);
			}
			break;

		default:
			burn_state = BURN_INIT;
			burn_timer = 0;
			break;
		}
	} while(loop);
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

/* igbt burn circuit v1.1 issues:
1)  mos gate over-protection needed
2) high/low voltage isolation needed
3) trig pin protection
4) current trig option? better for current sampling ....
*/
