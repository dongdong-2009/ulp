/*  capture.c
 *	dusk@2010 initial version
 */

#include "capture.h"
#include "stm32f10x.h"

/*
 *@initialize timer1 ch-1 to capture clock
 */
void capture_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	/* GPIOA clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	
	/* Configure PA.8 as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = 65535;	//defalut TIM_Period is 65535
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	//TIM_ARRPreloadConfig(TIM1, ENABLE);
	
	/*external clock mode 1 ,prescaler is off,clock edge is rising clock,input source is TI1_ED*/
	//TIM_ETRClockMode1Config(TIM1, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted,0);	
	TIM_TIxExternalClockConfig(TIM1, TIM_TIxExternalCLK1Source_TI1ED,TIM_ICPolarity_Rising, 0);
	TIM_SetCounter(TIM1,0);
	
	/*config and enable tim1 counter overflow interrupt*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
}

void capture_SetCounter(uint16_t count)
{
	TIM_Cmd(TIM1, DISABLE);
	//Sets the TIMx Autoreload Register value
	TIM_SetCounter(TIM1,count);
	TIM_Cmd(TIM1, ENABLE);
}

unsigned short capture_GetCounter(void)
{
	return TIM_GetCounter(TIM1);
}

void capture_ResetCounter(void)
{
	//TIM_GenerateEvent(TIM1,TIM_EventSource_Update);
	TIM_Cmd(TIM1, DISABLE);
	//Sets the TIMx Autoreload Register value
	TIM_SetCounter(TIM1,0);
	TIM_Cmd(TIM1, ENABLE);
}

void capture_SetAutoReload(unsigned short count)
{
	//Sets the TIMx Autoreload Register value
	TIM_SetAutoreload(TIM1,count);
}
unsigned short capture_GetAutoReload(void)
{
	//direct return ARR
	 return TIM1->ARR;
}

void capture_Start(void)
{
	TIM_Cmd(TIM1, ENABLE);
}

void capture_Stop(void)
{
	TIM_Cmd(TIM1, DISABLE);
}

void capture_SetCounterModeUp(void)
{
	TIM1->CR1 &= 0xEF;
}

void capture_SetCounterModeDown(void)
{
	TIM1->CR1 |= 0x10;
}
