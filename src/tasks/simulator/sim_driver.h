/*
 * David@2012 initial version
 */
#ifndef __WEIFU_H_
#define __WEIFU_H_
#include "stm32f10x.h"
#include "dac.h"

#define simulator_Start() do{clock_Enable(1);} while(0)
#define simulator_Stop() do{clock_Enable(0);} while(0)

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
	(((val) >= (min)) && ((val) <= (max)))

//clock reference functions
void clock_Init(void);
void clock_SetFreq(int hz);

//for sine wave output functions, 58x, oil pump...
void axle_Init(int option);  //such as 58x, 36x
void axle_SetFreq(int hz);
//void axle_SetAmp(short amp);
void op_Init(int option);    //such as 58x, 36x
//void op_SetAmp(short amp);

//api for vss and wss, 
void vss_Init(void);
void wss_Init(void);
void vss_SetFreq(short hz);
void wss_SetFreq(short hz);

//api for pwm output and counter input
//counter1 : TIM1_CH4
//counter2 : TIM2_CH2
//pwm1 : TIM3_CH3
//pwm2 : TIM4_CH2
void counter1_Init(void);
void counter2_Init(void);
int counter1_GetValue(void);
int counter2_GetValue(void);
void counter1_SetValue(int value);
void counter2_SetValue(int value);
void pwm1_Init(int frq, int dc);
void pwm2_Init(int frq, int dc);
void pwmin1_Init(void);
int pwmin1_GetDC(void);
int pwmin1_GetFrq(void);

void driver_Init(void);
void clock_Enable(int on);

//inline function define.
static inline void axle_SetAmp(short amp)
{
	volatile unsigned tmp = 0;

	tmp = DAC_BASE; 
	tmp += (unsigned)0x00000008 + DAC_Align_12b_R;
	/* Set the DAC channel1 selected data holding register */
	*(volatile unsigned *) tmp = amp;
}

static inline void op_SetAmp(short amp)
{
	volatile unsigned tmp = 0;

	tmp = DAC_BASE;
	tmp += (unsigned)0x00000014 + DAC_Align_12b_R;
	/* Set the DAC channel2 selected data holding register */
	*(volatile unsigned *)tmp = amp;
}

#endif /*__WEIFU_H_*/