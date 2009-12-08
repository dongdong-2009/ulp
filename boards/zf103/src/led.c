/* led.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x_lib.h"
#include "led.h"

void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable GPIOB clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* Configure PB.5 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
