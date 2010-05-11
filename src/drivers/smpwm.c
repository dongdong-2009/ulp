/*
*	dusk@2010 initial version
*/

#include "smpwm.h"
#include "stm32f10x.h"
/*
*default set:pwm frequency is 1KHZ,the duty rate %50,
*CH3 of TIM3
*/
void smpwm_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* Enable GPIOA,TIM3 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* Configure PB.0 as Output push-pull,TIM3-CH3*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = SM_PWM_PRIOD; //1k period
	TIM_TimeBaseStructure.TIM_Prescaler = 35; //1M clock to tim3
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel-3 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = SM_PWM_PRIOD>>1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);	
	TIM_ARRPreloadConfig(TIM3, ENABLE);
}

void smpwm_SetFrequency(int fre)
{

}

/*
 *duty:0-100
 */
void smpwm_SetDuty(int duty)
{
	int cmp;	
	
	cmp = SM_PWM_PRIOD*duty/100;
	TIM_SetCompare3(TIM3, cmp);
}
