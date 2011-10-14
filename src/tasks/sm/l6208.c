/*
*	dusk@2010 initial version
*/

#include "l6208.h"
#include "ulp_time.h"
#include <string.h>

void l6208_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    /* GPIOB clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Configure PB as output push pull*/
	GPIO_InitStructure.GPIO_Pin = L6208_RST | L6208_ENA | L6208_CTRL | L6208_HALF | L6208_DIR;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

#ifdef L6208_VREF_USE_PWM
/*set TIM3 CH3 as pwm output*/
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* Enable GPIOB,TIM3 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* Configure PB.0 as Output push-pull,TIM3-CH3*/
	GPIO_InitStructure.GPIO_Pin = L6208_VREF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Time base configuration,36 frequency division,PWM PERIOD is 10K*/
	TIM_TimeBaseStructure.TIM_Period = 99;
	TIM_TimeBaseStructure.TIM_Prescaler = 35;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* PWM3 Mode configuration Ch-3*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = (L6208_VREF_VOLTAGE*99)/3300;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	
//	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
#endif
}

/*
 *set the stepper motor direction
 */
void l6208_SetRotationDirection(int dir)
{
	if(dir == Clockwise ){
		GPIO_SetBits(GPIOB, L6208_DIR);
	}
	else{   
		GPIO_ResetBits(GPIOB, L6208_DIR);
	}
}

/*
 *set the stepper motor step mode,half or full
 */
void l6208_SelectMode(l6208_stepmode stepmode)
{
	if(stepmode == HalfMode ){
		GPIO_SetBits(GPIOB, L6208_HALF);
	}
	else{   
		GPIO_ResetBits(GPIOB, L6208_HALF);
	}
}

/*
 *set the stepper motor to home state
 */
void l6208_SetHomeState(void)
{
	int us = 1;
	GPIO_ResetBits(GPIOB,L6208_RST);
	while(us > 0) us--;
	GPIO_SetBits(GPIOB,L6208_RST);
}

/*
 *set the stepper motor decay mode,fast or slow
 */
void l6208_SetControlMode(l6208_controlmode controlmode)
{
	if(controlmode == DecaySlow ){
		GPIO_SetBits(GPIOB, L6208_CTRL);
	}
	else{   
		GPIO_ResetBits(GPIOB, L6208_CTRL);
	}
}

/*
 *enable or disable the stepper motor
 */
void l6208_StartCmd(FunctionalState state)
{
	if(state == ENABLE){
		GPIO_SetBits(GPIOB,L6208_ENA);
	}
	else{
		GPIO_ResetBits(GPIOB,L6208_ENA);
	}
}

