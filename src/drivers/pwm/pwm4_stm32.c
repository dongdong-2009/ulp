/*
*	dusk@2010 initial version
*	miaofng@2010 update as bus driver type
*/

#include "config.h"
#include "stm32f10x.h"
#include "pwm.h"

#define TIMn TIM4

static int pwm_init(const pwm_cfg_t *cfg)
{
	int f, div;
	RCC_ClocksTypeDef clks;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/*clock enable*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	RCC_GetClocksFreq(&clks);
	f = clks.PCLK1_Frequency;
	f <<= (clks.HCLK_Frequency == clks.PCLK1_Frequency) ? 0 : 1;
	div = f / cfg->hz / cfg -> fs;

	/* time base configuration */
	TIM_TimeBaseStructure.TIM_Period = cfg->fs;
	TIM_TimeBaseStructure.TIM_Prescaler = (div > 1) ? div - 1 : 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIMn, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIMn, ENABLE);
	TIM_Cmd(TIMn, ENABLE);
	return 0;
}

static int ch1_init(const pwm_cfg_t *cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	pwm_cfg_t def = PWM_CFG_DEF;

	cfg = (cfg == NULL) ? &def : cfg;
	pwm_init(cfg);

	/*config pin*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*config ch*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = cfg -> fs >> 1; //default to 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIMn, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIMn, TIM_OCPreload_Enable);
	return 0;
}

static int ch2_init(const pwm_cfg_t *cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	pwm_cfg_t def = PWM_CFG_DEF;

	cfg = (cfg == NULL) ? &def : cfg;
	pwm_init(cfg);

	/*config pin*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*config ch*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = cfg -> fs >> 1; //default to 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIMn, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIMn, TIM_OCPreload_Enable);
	return 0;
}

static int ch3_init(const pwm_cfg_t *cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	pwm_cfg_t def = PWM_CFG_DEF;

	cfg = (cfg == NULL) ? &def : cfg;
	pwm_init(cfg);

	/*config pin*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*config ch*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = cfg -> fs >> 1; //default to 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIMn, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIMn, TIM_OCPreload_Enable);
	return 0;
}

static int ch4_init(const pwm_cfg_t *cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	pwm_cfg_t def = PWM_CFG_DEF;

	cfg = (cfg == NULL) ? &def : cfg;
	pwm_init(cfg);

	/*config pin*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*config ch*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = cfg -> fs >> 1; //default to 50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIMn, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIMn, TIM_OCPreload_Enable);
	return 0;
}

static int ch1_set(int val)
{
	TIM_SetCompare1(TIMn, val);
	return 0;
}

static int ch2_set(int val)
{
	TIM_SetCompare2(TIMn, val);
	return 0;
}

static int ch3_set(int val)
{
	TIM_SetCompare3(TIMn, val);
	return 0;
}

static int ch4_set(int val)
{
	TIM_SetCompare4(TIMn, val);
	return 0;
}

const pwm_bus_t pwm41 = {
	.init = ch1_init,
	.set = ch1_set,
};

const pwm_bus_t pwm42 = {
	.init = ch2_init,
	.set = ch2_set,
};

const pwm_bus_t pwm43 = {
	.init = ch3_init,
	.set = ch3_set,
};

const pwm_bus_t pwm44 = {
	.init = ch4_init,
	.set = ch4_set,
};
