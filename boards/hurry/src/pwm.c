/* pwm.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "pwm.h"

/*pin26 PB0_ADC_IN8_TIM3_CH3_SOUND*/
void pwm_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* Enable GPIOB clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Configure PB.0 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Configure PB.1 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = (1000 - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = (72000/1000 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel3 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 800;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	/* PWM1 Mode configuration: Channel4 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 800;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	TIM_ARRPreloadConfig(TIM3, ENABLE);
}

void pwm_update(void)
{
}

/*freq: Hz, duty: 0~100*/
void pwm_config(int freq, int duty)
{
	int pre, cnt, cmp;
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);

	pre = 1;
	cnt = RCC_Clocks.PCLK1_Frequency/freq;
	if( cnt > 100 ) {
		cnt = 100;
		pre = RCC_Clocks.PCLK1_Frequency/freq/cnt;
	}
	cmp = cnt*duty/100;

	TIM_PrescalerConfig(TIM3, pre-1, TIM_PSCReloadMode_Update);
	TIM_SetCounter(TIM3, cnt-1);
	TIM_SetCompare3(TIM3, cmp);
}

#ifdef PWM_DEBUG
/*
*default set:pwm frequency is 1KHZ,the duty rate %50,
*CH1,CH2,CH3,CH4 of TIM3 are valid and with the same config
*/
void pwmdebug_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	/* Enable GPIOA,GPIOB,TIM3 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/* Configure PA.6 as Output push-pull,TIM3-CH1 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure PA.7 as Output push-pull,TIM3-CH2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	/* Configure PB.0 as Output push-pull,TIM3-CH3 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* Configure PB.1 as Output push-pull,TIM3-CH4 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//!!!
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = (1000 - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = (72 - 1);//get 1M Tim3 counter clock
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	/* PWM1 Mode configuration: Channel2 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	/* PWM1 Mode configuration: Channel3 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	/* PWM1 Mode configuration: Channel4 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 500;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	TIM_ARRPreloadConfig(TIM3, ENABLE);

}

/*freq: Hz, duty: 0~100,prescaler is 72,Tim counter clock = 1M*/
void pwmdebug_config(uint16_t ch,int freq, int duty)
{
	int cnt, cmp;
//	RCC_ClocksTypeDef RCC_Clocks;
//	RCC_GetClocksFreq(&RCC_Clocks);

	cnt = 1000000/freq;
	
	if(duty>100)
		cmp = cnt;
	else if(duty<0)
		cmp = 0;
	else
		cmp = cnt*duty/100;

//	TIM_PrescalerConfig(TIM3, pre-1, TIM_PSCReloadMode_Update);
	TIM_SetCounter(TIM3, cnt-1);
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