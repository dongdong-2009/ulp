/*
*	david@2012 initial version
*/

#include <stdio.h>
#include "config.h"
#include "stm32f10x.h"
#include "pwmin.h"
#include "ulp_time.h"

#define TIMn TIM1

static volatile int pwmin_dc;
static volatile int pwmin_frq;
static int SAMPLE_CLOCK;
static volatile time_t pwmin_update_timer;

static int pwmin_init(const pwmin_cfg_t *cfg)
{
	int f, div;
	RCC_ClocksTypeDef clks;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/*clock enable*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* TIM1 channel 1 pin (PA.08) configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	SAMPLE_CLOCK = cfg -> sample_clock;
	RCC_GetClocksFreq(&clks);
	f = clks.PCLK2_Frequency;
	div = f / cfg -> sample_clock;

	/* time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = div - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInit(TIMn, &TIM_TimeBaseStructure);

	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_PWMIConfig(TIMn, &TIM_ICInitStructure);
	//* Select the TIM1 Input Trigger: TI1FP1
	TIM_SelectInputTrigger(TIMn, TIM_TS_TI1FP1);
	/* Select the slave Mode: Reset Mode */
	TIM_SelectSlaveMode(TIMn, TIM_SlaveMode_Reset);
	/* Enable the Master/Slave Mode */
	TIM_SelectMasterSlaveMode(TIMn, TIM_MasterSlaveMode_Enable);
	TIM_Cmd(TIMn, ENABLE);
	TIM_ITConfig(TIMn, TIM_IT_CC1, ENABLE);//* Enable the CC1 Interrupt Request

	/* Enable the TIM3 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	pwmin_update_timer = time_get(1000);

	return 0;
}

static int getdc(void)
{
	if (time_left(pwmin_update_timer) > 0) {
		return pwmin_dc;
	} else
		return 0;
}

static int getfrq(void)
{
	if (time_left(pwmin_update_timer) > 0)
		return pwmin_frq;
	else
		return 0;
}

void TIM1_CC_IRQHandler(void)
{
	int ic_value;
	pwmin_update_timer = time_get(1000);

	/* Clear TIM3 Capture compare interrupt pending bit */
	TIM_ClearITPendingBit(TIMn, TIM_IT_CC1);

	/* Get the Input Capture value */
	ic_value = TIM_GetCapture1(TIMn);

	if (ic_value != 0) {
		/* Duty cycle computation */
		pwmin_dc = (TIM_GetCapture2(TIMn) * 100) / ic_value;

		/* Frequency computation */
		pwmin_frq = SAMPLE_CLOCK / ic_value;
	} else {
		pwmin_dc = 0;
		pwmin_frq = 0;
	}
}

const pwmin_bus_t pwmin11 = {
	.init = pwmin_init,
	.getdc = getdc,
	.getfrq = getfrq,
};
