/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "led.h"
#include "stm32f10x.h"

void led_hwInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1)
	/* Enable GPIOA,GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  
	/* Configure PC.10 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
	/* Configure PC.12 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#else
	/*led_r pg8, led_g pg15*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
#endif
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	BitAction ba;
	switch(status) {
		case LED_OFF:
			ba = Bit_RESET;
			break;
		case LED_ON:
			ba = Bit_SET;
			break;
		default:
			return;
	}
	
	switch(led) {
		case LED_GREEN:
#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_12, ba);
#else
			GPIO_WriteBit(GPIOG, GPIO_Pin_15, ba);
#endif
			break;
		case LED_RED:
#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_10, ba);
#else
			GPIO_WriteBit(GPIOG, GPIO_Pin_8, ba);
#endif
			break;
		default:
			break;
	}
}