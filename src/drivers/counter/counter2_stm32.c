/*
*	dusk@2012 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "counter.h"

#define TIMn TIM2

static void counter_init(const counter_cfg_t *cfg)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 65535;	//defalut TIM_Period is 65535
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMn, &TIM_TimeBaseStructure);
}

static void ch1_init(const counter_cfg_t *cfg)
{
	counter_cfg_t def = COUNTER_CFG_DEF;
	cfg = (cfg == NULL) ? &def : cfg;
	counter_init(cfg);

	/* GPIOA clock enable */
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* Configure PA.0 as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*external clock mode 1 ,prescaler is off,clock edge is rising clock,input source is TI1*/
	TIM_TIxExternalClockConfig(TIMn, TIM_TIxExternalCLK1Source_TI1,TIM_ICPolarity_Rising, 0);
	TIM_SetCounter(TIMn,0);
	TIM_Cmd(TIMn, ENABLE);
}

static void ch2_init(const counter_cfg_t *cfg)
{
	counter_cfg_t def = COUNTER_CFG_DEF;
	cfg = (cfg == NULL) ? &def : cfg;
	counter_init(cfg);

	/* GPIOA clock enable */
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* Configure PA.1 as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*external clock mode 1 ,prescaler is off,clock edge is rising clock,input source is TI2*/
	TIM_TIxExternalClockConfig(TIMn, TIM_TIxExternalCLK1Source_TI2,TIM_ICPolarity_Rising, 0);
	TIM_SetCounter(TIMn,0);
	TIM_Cmd(TIMn, ENABLE);
}

static void counter_set(int val)
{
	TIM_SetCounter(TIMn,val);
}

static int counter_get(void)
{
	return TIM_GetCounter(TIMn);
}

const counter_bus_t counter21 = {
	.init = ch1_init,
	.get = counter_get,
	.set = counter_set,
};

const counter_bus_t counter22 = {
	.init = ch2_init,
	.get = counter_get,
	.set =counter_set,
};
