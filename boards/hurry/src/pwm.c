/* pwm.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "pwm.h"

/*
*note:TIM1 counter default value = 1000,just TIM1_COUNTER_VALUE
*so the PWM carrier frequency is between 72KHz and 1.098Hz
*/

/*
*TIM1 default init:TIM1 counter = 1000 ,duty cycle = 50%
*/
void pwm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
	
	/* Enable GPIOA,GPIOB,TIM1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* Configure PA.8,PA.9,PA.10 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure PB.13,PB.14,PB.15 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Time base configuration ,Pattern type is center aligned */
	TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PWM_PRSC;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	// Initial condition is REP=0 to set the UPDATE only on the underflow
	TIM_TimeBaseStructure.TIM_RepetitionCounter = REP_RATE;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
	TIM_ARRPreloadConfig(TIM1, ENABLE);
	
	/* Automatic Output enable, Break, dead time and lock configuration*/
	TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
	TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
	TIM_BDTRInitStructure.TIM_DeadTime = DEADTIME;
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
	
	/* Master Mode selection */
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);

	/* PWM2 Mode configuration: Channel1 Channel2 Channel3*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = PWM_PERIOD/2;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;	

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
	
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);	
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

#if 0
	// Clear Update Flag
	TIM1_ClearFlag(TIM1_FLAG_Update);
	TIM1_ITConfig(TIM1_IT_Update, DISABLE);
#endif  
	/* Master Mode selection */
	//TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);

	/* Main Output Enable */
	//TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

void pwm_Update(void)
{
}

/* ch is u,v,w ch duty: 0-100 repetition :0-255*/
void pwm_Config(unsigned short ch, int duty, unsigned char repetition)
{
	int cmp;

	TIM1->RCR = repetition;
	cmp = PWM_PERIOD*duty/100;

	switch(ch){
		case PWM_U:
		TIM_SetCompare1(TIM1, cmp);
		break;
		case PWM_V:
		TIM_SetCompare2(TIM1, cmp);
		break;
		case PWM_W:
		TIM_SetCompare3(TIM1, cmp);
		break;
		default:
		break;
	}
}

#ifdef PWM_DEBUG
/*
*default set:pwm frequency is 1KHZ,the duty rate %50,
*CH1,CH2,CH3,CH4 of TIM3 are valid and with the same config
*/
void pwmdebug_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* Enable GPIOA,GPIOB,TIM3 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* Configure PA.6 PA.7 as Output push-pull,TIM3-CH1 TIM3-CH2*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure PB.0 PB.1 as Output push-pull,TIM3-CH3 TIM3-CH4*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler =PWM_PRSC;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel1,Channel2,Channel3,Channel4 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = PWM_PERIOD/2;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	
	
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);	
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	TIM_ARRPreloadConfig(TIM3, ENABLE);
}

/* ch is ch1,ch2,ch3,ch4 ch duty: 0-100 */
void pwmdebug_Config(unsigned short ch, int duty)
{
	int cmp;	
	
	cmp = PWM_PERIOD*duty/100;

	switch(ch){
		case PWMDEBUG_CH1:
		TIM_SetCompare1(TIM3, cmp);
		break;
		case PWMDEBUG_CH2:
		TIM_SetCompare2(TIM3, cmp);
		break;
		case PWMDEBUG_CH3:
		TIM_SetCompare3(TIM3, cmp);
		break;
		case PWMDEBUG_CH4:
		TIM_SetCompare4(TIM3, cmp);
		break;
		default:
		break;
	}
}
#endif