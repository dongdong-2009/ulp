/* this project is used to emulate vehicle spark waveform, then measure the peak current and voltage,
according to these two value, dut igbt performance cauld be identified.
there are 4 hardware modules in total:
1, pmm - period measure module & timing unit --- HW TIMER - TIM4
2, mos - spark discharge control module, core of spark waveform control --- HW TIMER - TIM2
3, ipm - spark current measure module --- ADC1 DMA
4, vpm - spark voltage measure module --- TIM3 DMA

modification history:
miaofng@2011	initial version
miaofng@2012	spark period adaptive fit (0 - 65mS)
*/
#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "nvm.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "stm32f10x.h"
#include <string.h>
#include "priv/mcamos.h"
#include "lib/nest_burn.h"
#include "shell/cmd.h"
#include <stdlib.h>
#include "crc.h"

#define CONFIG_IGBT_CRC 1
#define HARDWARE_VERSION	0x0104 //v1.4

#if HARDWARE_VERSION == 0x0104
//R = 0.01Ohm, G = 20V/V, Vref = 3V3, 16bit unsigned ADC => ratio = 1/3971.88= 256/1016801
#define ipm_ratio_def 1016
#else
//R = 0.01Ohm, G = 20V/V, Vref = 2V5, 16bit unsigned ADC => ratio = 1/5242.88= 256/1342177
#define ipm_ratio_def 1342
#endif
#if HARDWARE_VERSION == 0x0101
//R18 = 22K, R17 = 47Ohm, Vref = 2V, 16bit unsigned ADC => ratio = 1/69.85513 = 256/17883
#define vpm_ratio_def 17883
#else
//R18 = 22K, R17 = 100Ohm, Vref = 2V, 16bit unsigned ADC => ratio = 1/148.2715= 256/37957
#define vpm_ratio_def 37957
#endif

#define T			20 //spark period, unit: ms
#define ipm_ms			4 //norminal ipm measurement time
#define vpm_ms			1 //norminal vpm measurement time
#define mos_delay_us_def	1.7 //range: 1/36 ~ 65535/36 uS
#define mos_close_us_def	1000 //range: 1/36 ~ 65535/36 uS

//rough waveform data, captured by vpm_Update & ipm_Update
#define VPM_FIFO_N	256
#define IPM_FIFO_N	256
static unsigned short vpm_data[VPM_FIFO_N];
static unsigned short ipm_data[IPM_FIFO_N];
static short vpm_data_n = 0;
static short ipm_data_n = 0;
static int vpm_ratio_cal __nvm; //[1/4096 - 255]
static int ipm_ratio_cal __nvm; //[1/4096 - 255]
static int vpm_ratio; //vp(v) = vp(digit) * 256 / ratio
static int ipm_ratio; //ip(mA) = ip(digit) * 256 / ratio

static unsigned short mos_delay_clks __nvm;
static unsigned short mos_close_clks __nvm;
static unsigned short burn_ms __nvm;
static time_t burn_timer;
static time_t burn_tick; //for debug purpose

static int burn_id __nvm; /*mcamos can cmd id*/

/*burn system state machine*/
static enum {
	BURN_INIT,
	BURN_MEAT,
	BURN_IDLE, //1st pulse detected, idle duration, normal 36mS
	BURN_MEAI, //current measurement duration, normal 4mS
	BURN_MEAV, //voltage measurement duration, normal < 1mS
} burn_state;

/*burn ops*/
enum {
	START,
	UPDATE,
	STOP,
};

static struct burn_data_s burn_data;

struct filter_s {
	int b0, b1, bn; //num
	int a0, a1, an; //den
	int xn_1, yn_1;
} burn_filter_vp, burn_filter_ip;

#define BURN_FILTER_FP_HZ	0.5 /*filter pass band freq, unit: Hz(note: fs = 50Hz)*/
#define BURN_FILTER_DELAY	((int)(5000 / BURN_FILTER_FP_HZ)) /*settling time, unit: mS*/

void filter_init(struct filter_s *f)
{
	f->bn = 19/*24*/;
	f->an = 14;

	f->b0 = (int)(0.030468747091253801/*0.0006279240770159511*/ * (1 << f->bn));
	f->b1 = (int)(0.030468747091253801/*0.0006279240770159511*/ * (1 << f->bn));
	f->a0 = (int)(1.0000000000000000000 * (1 << f->an));
	f->a1 = (int)(-0.9390625058174924/*-0.9987441518459681*/ * (1 << f->an));

	f->xn_1 = 0;
	f->yn_1 = 0;
}

int filt(struct filter_s *f, int xn)
{
	int vb, va;
	vb = f->b0 * xn;
	vb += f->b1 * f->xn_1;
	vb >>= f->bn;

	va = f->a1 * f->yn_1;
	va = 0 - va;
	va >>= f->an;

	vb += va;

	f->xn_1 = xn;
	f->yn_1 = vb;
	return vb;
}

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

	TIM_Cmd(TIM2, DISABLE);

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
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; /*bc847+zxgd3003->lm5109b*/
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

enum {
	FLAG_DEBUG_NONE, //disp nothing
	FLAG_DEBUG_VP, //disp trape waveform data
	FLAG_DEBUG_IP, //disp triangle waveform data
	FLAG_DEBUG_VI, //disp V(v)&I(mA)
	FLAG_DEBUG_CAN,
} burn_flag_debug = FLAG_DEBUG_NONE;

int cmd_igbt_func(int argc, char *argv[])
{
	if(argc == 4) {
		if(!strcmp(argv[1], "vp")) {
			int ve = atoi(argv[2]); //expect
			int vr = atoi(argv[3]); //real
			vpm_ratio_cal = (ve << 12) / vr;
			vpm_ratio = (vpm_ratio_def * vpm_ratio_cal) >> 12;
			return 0;
		}
		if(!strcmp(argv[1], "ip")) {
			int ie = atoi(argv[2]); //expect
			int ir = atoi(argv[3]); //real
			ipm_ratio_cal = (ie << 12) / ir;
			ipm_ratio = (ipm_ratio_def * ipm_ratio_cal) >> 12;
			return 0;
		}
	}

	if(argc == 3) {
		if(!strcmp(argv[1], "id")) {
			sscanf(argv[2], "0x%x", &burn_id);
			return 0;
		}

		if(!strcmp(argv[1], "vp")) {
			vpm_ratio_cal = atoi(argv[2]);
			vpm_ratio = (vpm_ratio_def * vpm_ratio_cal) >> 12;
			return 0;
		}

		if(!strcmp(argv[1], "ip")) {
			ipm_ratio_cal = atoi(argv[2]);
			ipm_ratio = (ipm_ratio_def * ipm_ratio_cal) >> 12;
			return 0;
		}

		if(!strcmp(argv[1], "wp")) {
			int nclk = atoi(argv[2]);
			int delay_clks = nclk + 25; /*25 * 1000 / 36 = 0.694 uS, trig delay*/
			int total = delay_clks + mos_close_clks;
			int close_clks = (total > 0xfff0) ? (0xfff0 - delay_clks) : mos_close_clks;

			mos_delay_clks = (short) delay_clks;
			mos_close_clks = (short) close_clks;

			mos_Init();
			return 0;
		}

		if(!strcmp(argv[1], "debug")) {
			if(!strcmp(argv[2], "vp"))
				burn_flag_debug = FLAG_DEBUG_VP;
			else if(!strcmp(argv[2], "ip"))
				burn_flag_debug = FLAG_DEBUG_IP;
			else if(!strcmp(argv[2], "vi"))
				burn_flag_debug = FLAG_DEBUG_VI;
			else if(!strcmp(argv[2], "can"))
				burn_flag_debug = FLAG_DEBUG_CAN;
			else
				burn_flag_debug = FLAG_DEBUG_NONE;
			return 0;
		}
	}

	if(argc == 2) {
		if(!strcmp(argv[1], "save")) {
			nvm_save();
			return 0;
		}
		else if(!strcmp(argv[1], "ical")) {
			int val, i, fil, avg;
			struct filter_s f, f2;
			filter_init(&f);
			filter_init(&f2);

			ADC_Cmd(ADC1, ENABLE);
			ADC_SoftwareStartConvCmd(ADC1, ENABLE);
			while(1) {
				ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
				while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
				val = ADC_GetConversionValue(ADC1);
				i = (val << 8) / ipm_ratio;
				if(i > 500) { //bigger than 0.5A
					fil = filt(&f, val);
					avg = filt(&f2, fil);
					i = (avg << 8) / ipm_ratio;
					printf("%d %d %d %d\n", val, fil, avg, i);
					mdelay(10);
				}
			}
		}
	}

	printf(
		"igbt wp nclk		set vpeak pulse width = nclk * 27.8nS(1/36MHz)\n"
		"igbt id xx		set mcamos server can id, such as 0x5e0/2/4/6\n"
		"igbt ical		current calibration mode\n"
		"igbt debug vp/ip/vi/can	disp trape waveform/triangle waveform/peak VI waveform\n"
		"igbt vp ratio		(vm * ratio) >> 12 = ve\n"
		"igbt vp ve vm		vexpect vmeasure, unit: V\n"
		"igbt ip ratio		(im * ratio) >> 12 = ie\n"
		"igbt ip ie im		iexpect imeasure, unit: mA\n"
		"igbt save		save the config value\n"
	);

	printf("\ncurrent nvm settings:\n");
	printf("burn_wp = %d\n", mos_delay_clks - 25);
	printf("burn_ms = %d\n", burn_ms);
	printf("burn_id = 0x%x\n", burn_id);
	printf("vpm_ratio_cal = %d\n", vpm_ratio_cal);
	printf("ipm_ratio_cal = %d\n", ipm_ratio_cal);
	return 0;
}

const cmd_t cmd_igbt = {"igbt", cmd_igbt_func, "igbt board cmds"};
DECLARE_SHELL_CMD(cmd_igbt)

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
	TIM_TimeBaseStructure.TIM_Period =  11; //Fclk = 72Mhz / 12 = 6Mhz
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* TIM2 PWM2 Mode configuration: Channel1 */
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 6; //duty factor = 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //down going edge sampling(up going edge 9203 DO changes)
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
#if HARDWARE_VERSION == 0x0101
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
#else
#define vpm_correct(x) (x)
#endif

int idx(int n, int sz)
{
	n += (n > 0) ? 0 : sz;
	for(; n >= sz; n -= sz);
	return n;
}

void vpm_Update(int ops)
{
	int n, i, vp, vp_max = 0;

	switch(ops) {
	case START:
		//vpm_data_n = VPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel6);
		break;
	case UPDATE:
		break;
	case STOP:
		n = VPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel6);
		if(burn_flag_debug == FLAG_DEBUG_VP) {
			i = (n > vpm_data_n) ? (n - vpm_data_n) : (VPM_FIFO_N - vpm_data_n + n);
			printf("vpm_data[%d] = \n", i);
			if(n > vpm_data_n) { //normal
				for(i = vpm_data_n; i < n; i ++)
				printf("%d ", vpm_correct(vpm_data[i]));
			}
			else {
				for(i = vpm_data_n; i < VPM_FIFO_N; i ++)
					printf("%d ", vpm_correct(vpm_data[i]));
				for(i = 0; i < n; i ++)
					printf("%d ", vpm_correct(vpm_data[i]));
			}
			printf("\n");
		}

		//calculate peak voltage(6MSPS)
		i = (n > vpm_data_n) ? (n - vpm_data_n) : (VPM_FIFO_N - vpm_data_n + n);
		vpm_data_n = n;
		for(; i > 0; i --) {
			n = idx(vpm_data_n - i, VPM_FIFO_N);
			vp = vpm_correct(vpm_data[n]);
			vp_max = (vp > vp_max) ? vp : vp_max;
		}

		vp = filt(&burn_filter_vp, vp_max);
		burn_data.vp = (vp_max << 8) / vpm_ratio;
		burn_data.vp_avg = (vp << 8) / vpm_ratio;

		if(- time_left(burn_tick ) > BURN_FILTER_DELAY) {
			burn_data.vp_max = (burn_data.vp_avg > burn_data.vp_max) ? burn_data.vp_avg : burn_data.vp_max;
			burn_data.vp_min = (burn_data.vp_avg < burn_data.vp_min) ? burn_data.vp_avg : burn_data.vp_min;
		}
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

	//ADC_DMACmd(ADC1, ENABLE);

	//awd & irq config
	ADC_AnalogWatchdogThresholdsConfig(ADC1, 0x1F0,0x000);  //trig 2A
	ADC_AnalogWatchdogSingleChannelConfig(ADC1, ADC_Channel_5);
	ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);
	ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*analog watchdog isr*/
void ADC1_2_IRQHandler(void)
{
	ADC_DMACmd(ADC1, ENABLE); //enable DMA transfer
	ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
	ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
}

void ipm_Update(int ops)
{
	int n, i, ip, ip_max;

	switch(ops) {
	case START:
		ADC_Cmd(ADC1, ENABLE);
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
		ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
		break;
	case UPDATE:
		break;
	case STOP:
		ADC_DMACmd(ADC1, DISABLE);
		ADC_Cmd(ADC1, DISABLE);
		n = IPM_FIFO_N - DMA_GetCurrDataCounter(DMA1_Channel1);
		if(burn_flag_debug == FLAG_DEBUG_IP) {
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

		//calculate peak voltage(6MSPS)
		i = (n > ipm_data_n) ? (n - ipm_data_n) : (IPM_FIFO_N - ipm_data_n + n);
		ipm_data_n = n;
		for(; i > 0; i --) {
			n = idx(ipm_data_n - i, IPM_FIFO_N);
			ip = ipm_data[n];
			ip_max = (ip > ip_max) ? ip : ip_max;
		}

		ip = filt(&burn_filter_ip, ip_max);
		burn_data.ip = (ip_max << 8) / ipm_ratio;
		burn_data.ip_avg = (ip << 8) / ipm_ratio;
		if(- time_left(burn_tick ) > BURN_FILTER_DELAY) {
			burn_data.ip_max = (burn_data.ip_avg > burn_data.ip_max) ? burn_data.ip_avg : burn_data.ip_max;
			burn_data.ip_min = (burn_data.ip_avg < burn_data.ip_min) ? burn_data.ip_avg : burn_data.ip_min;
		}
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

	//TRIG IN(low effective)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
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
	/* TIM4 CH1 = Input Capture Mode, low effective */
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x15;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);

	/* TIM4 CH2 = Input Capture Mode , high effective*/
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

//para ignore is used to avoid error trig signal in some duration
int pmm_Update(int ops, int ignore)
{
	int f, t1, t2, wp, tp, ms;
	static int pmm_t1;
	static time_t pmm_t1_ms;

	f = TIM4->SR;
	if(ignore) { //to clr err trig isr flag
		TIM4->SR = ~f;
		return 0;
	}

	if((f & TIM_FLAG_CC1) && (f & TIM_FLAG_CC2)) {
		t1 = TIM4->CCR2; /*upgoing edge of firepulse*/
		t2 = TIM4->CCR1; /*dngoing edge of firepulse*/
		TIM4->SR = ~f;

		//fire pulse width, rational range: 1-200uS
		wp = t2 - t1;
		wp += (wp < 0) ? pmm_T : 0;

		//fire pulse period, rational range: 10-500mS
		ms = - time_left(pmm_t1_ms); /*tp measured by sys tick*/
		pmm_t1_ms = time_get(0); /*!!!update burn_tick*/
		tp = t1 - pmm_t1;
		tp += (tp < 0) ? pmm_T : 0;
		pmm_t1 = t1; /*!!!update pmm_t1*/

		if(ops == START) {
			burn_tick = pmm_t1_ms;
			return 1;
		}

		//tp measured by hw timer, timer overflow??? insert n*pmm_T
		ms += 10; /*sys tick jitter err = +/-10mS*/
		#define __ms(us) ((us)/1000)
		while(__ms(tp + pmm_T) < ms) {
			tp += pmm_T;
		}

		//update burn_data
		burn_data.wp = wp * 1000;
		burn_data.tp = __ms(tp);
		return 1;
	}
	return 0;
}

static mcamos_srv_t burn_server;

/*communication with host*/
void com_Init(void)
{
	burn_server.can = &can1;
	burn_server.id_cmd = burn_id;
	burn_server.id_dat = burn_id + 1;
	burn_server.baud = BURN_BAUD;
	burn_server.timeout = 8;
	burn_server.inbox_addr = BURN_INBOX_ADDR;
	burn_server.outbox_addr = BURN_OUTBOX_ADDR;
	mcamos_srv_init(&burn_server);
}

void com_Update(void)
{
	char ret = 0;
	char *inbox = burn_server.inbox;
	char *outbox = burn_server.outbox + 2;
	struct burn_cfg_s *pcfg;

	mcamos_srv_update(&burn_server);
	switch(inbox[0]) {
	case BURN_CMD_CONFIG:
		pcfg = (struct burn_cfg_s *) inbox;
		if(pcfg->vcal != 0) vpm_ratio_cal = pcfg->vcal;
		if(pcfg->ical != 0) ipm_ratio_cal = pcfg->ical;
		if(pcfg->tp != 0) burn_ms = pcfg->tp;
		if(pcfg->wp != 0) {
			int nclk = pcfg->wp * 36 / 1000;
			int delay_clks = nclk + 25; /*25 * 1000 / 36 = 0.694 uS, trig delay*/
			int total = delay_clks + mos_close_clks;
			int close_clks = (total > 0xfff0) ? (0xfff0 - delay_clks) : mos_close_clks;

			mos_delay_clks = (short) delay_clks;
			mos_close_clks = (short) close_clks;
		}

		void burn_Init();
		burn_Init();
		if(inbox[1] == 's') nvm_save();
		break;

	case BURN_CMD_READ:
		if(burn_flag_debug == FLAG_DEBUG_CAN) {
			printf("R: %dV %dmA %dnS\n", burn_data.vp_avg, burn_data.ip_avg, burn_data.wp);
		}
		#ifdef CONFIG_IGBT_CRC
			burn_data.crc_flag = (unsigned short) burn_id;
			burn_data.crc = 0;
			burn_data.crc = cyg_crc16((unsigned char *) &burn_data, sizeof(burn_data));
		#endif
		if(burn_data.lost != 0) printf("lost = %d\n", burn_data.lost);
		memcpy(outbox, &burn_data, sizeof(burn_data));
		break;

	case 0:
		return;
	}

	burn_server.outbox[0] = inbox[0];
	burn_server.outbox[1] = ret;
	inbox[0] = 0; //clear inbox testid indicate cmd ops finished!
}

void burn_Init()
{
	filter_init(&burn_filter_vp);
	filter_init(&burn_filter_ip);
	burn_state = BURN_INIT;
	memset(&burn_data, 0, sizeof(burn_data));
	burn_data.ip_min = 0xffff;
	burn_data.vp_min = 0xffff;

	static int prst = 1; //flag of power-on-reset
	if(nvm_is_null() && prst) { //set default value
		mos_delay_clks  = (unsigned short) (mos_delay_us_def * 36); //unit: 1/36 us
		mos_close_clks  = (unsigned short) (mos_close_us_def * 36); //unit: 1/36 us
		burn_ms = T;
		vpm_ratio_cal = 1 << 12;
		ipm_ratio_cal = 1 << 12;
		nvm_save();
	}

	burn_ms = 0; //adaptive spark period measurement, ignore obsolete nvm settings
	vpm_ratio_cal = (vpm_ratio_cal > 0) ? vpm_ratio_cal : 1 << 12;
	ipm_ratio_cal = (ipm_ratio_cal > 0) ? ipm_ratio_cal : 1 << 12;

	vpm_ratio = (vpm_ratio_cal * vpm_ratio_def) >> 12;
	ipm_ratio = (ipm_ratio_cal * ipm_ratio_def) >> 12;

	pmm_Init();
	mos_Init();
	if(prst) com_Init();
	ipm_Init();
	vpm_Init();
	prst = 0;
}

void Analysis(void)
{
	if(burn_flag_debug == FLAG_DEBUG_VI) {
		int ori, avg, min, max;
		ori = burn_data.vp;
		avg = burn_data.vp_avg;
		min = burn_data.vp_min;
		max = burn_data.vp_max;
		printf("%d %d %d %d	", ori, avg, min, max);

		ori = burn_data.ip;
		avg = burn_data.ip_avg;
		min = burn_data.ip_min;
		max = burn_data.ip_max;
		printf("%d %d %d %d	", ori, avg, min, max);

		int wp = burn_data.wp;
		int fire = burn_data.fire;
		int lost = burn_data.lost;
		printf("%d %d %d\n", wp, fire, lost);
	}
}

static unsigned short burn_ms_temp;
static unsigned short burn_ms_times;

void burn_Update()
{
	int flag, loop, delta;
	com_Update();
	do {
		loop = 0;
		switch(burn_state) {
		case BURN_MEAT: //spark period measurement
			flag = pmm_Update(UPDATE, 0);
			if(flag) {
				if(burn_ms_times == 0) {
					burn_ms_temp = burn_data.tp;
					burn_ms_times ++;
				}
				else {
					delta = burn_data.tp;
					delta -= burn_ms_temp;
					if((delta < 3) && (delta > -3)) { // <3mS, the same one
						delta /= 2; //to got average value
						delta += burn_ms_temp;
						burn_ms_temp = delta;
						burn_ms_times ++;
						if(burn_ms_times > 5) { //enough ...
							burn_ms = burn_ms_temp;
							burn_state = BURN_INIT;
						}
					}
					else { //not stable, new one got
						burn_ms = 0;
						burn_state = BURN_INIT;
					}
				}
			}
			break;

		case BURN_INIT:
			flag = pmm_Update(START, 0);
			if(flag) {
				if(burn_ms != 0) {
					burn_state = BURN_IDLE;
					burn_timer = time_get(burn_ms - ipm_ms);
				}
				else {
					burn_state = BURN_MEAT;
					burn_ms_temp = 0;
					burn_ms_times = 0;
				}
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
				burn_data.fire ++;
				break;
			}
			if(time_left(burn_timer) < 0) { //lost trig signal at this cycle??  resync
				burn_state = BURN_INIT;
				burn_timer = 0;
				burn_data.lost ++;
				burn_data.vp = burn_data.vp_avg = 0;
				burn_data.ip = burn_data.ip_avg = 0;
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
				burn_timer = time_get(burn_ms + time_left(burn_timer) - ipm_ms);
				vpm_Update(STOP);
				Analysis();
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
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	while(1) {
		task_Update();
		burn_Update();
	}
}

/*
igbt burn board v1.1 issues:
1)  mos gate over-protection needed -- external smps power issue
2) high/low voltage isolation needed -- solved, discharge path changed
3) trig pin protection -- clamp diode added
4) current trig option? better for current sampling .... not used

igbt burn board v1.2 issues:
1) screw hole isolation area not big enough
2) mos heat issue
3) mos on delay issue, R73: 4k7 -> 470, R72 Package 1206?
4) reset issue, reset cap: 100n->10u, remove reset resistor 10k pull up
5) mos alignment issue, not in a line
6) 12v dcdc package silk mirror issue
7) groud relief connection issue, difficult to soldering
8) plastic handle issue
*/
