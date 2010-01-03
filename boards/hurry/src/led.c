/* led.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "led.h"
#include "time.h"

void led_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
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
}

#define LED_FLASH_TIMER_STOP 0
static time_t led_flash_timer[NR_OF_LED];
static int led_status;

void led_Update(void)
{
	int i, left;
	time_t time;
	
	for(i = 0; i < NR_OF_LED; i++)
	{
		time = led_flash_timer[i];
		if( time != LED_FLASH_TIMER_STOP)
		{
			left = time_left(time);
			if( left < 0) {
				led_inv((led_t)i);
				time = time_get(left + LED_FLASH_PERIOD);
				led_flash_timer[i] = time;
			}
		}
	}
}

void led_SetStatus(led_t led, led_status_t status)
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
			//GPIO_SetBits(GPIOB, GPIO_Pin_5);
			GPIO_WriteBit(GPIOC, GPIO_Pin_10, ba);
			break;
		case LED_YELLOW:
			GPIO_WriteBit(GPIOC, GPIO_Pin_12, ba);
			break;
		default:
			break;
	}
}

void led_on(led_t led)
{
	int i = (int)led;
	led_flash_timer[i] = LED_FLASH_TIMER_STOP;
	led_status |= (1 << i);
	led_SetStatus(led, LED_ON);
}

void led_off(led_t led)
{
	int i = (int)led;
	led_flash_timer[i] = LED_FLASH_TIMER_STOP;
	led_status &= ~(1 << i);
	led_SetStatus(led, LED_OFF);
}

void led_inv(led_t led)
{
	int i = (int)led;
	int mask = 1 << i;
	
	if(led_status&mask) 
		led_off(led);
	else
		led_on(led);
}

void led_flash(led_t led)
{
	int i = (int)led;
	led_off(led);
	led_flash_timer[i] = time_get(LED_FLASH_PERIOD);
}
