/* led.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "led.h"
#include "time.h"

void led_Init(void)
{
	led_hwInit();
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

void led_on(led_t led)
{
	int i = (int)led;
	led_flash_timer[i] = LED_FLASH_TIMER_STOP;
	led_status |= (1 << i);
	led_hwSetStatus(led, LED_ON);
}

void led_off(led_t led)
{
	int i = (int)led;
	led_flash_timer[i] = LED_FLASH_TIMER_STOP;
	led_status &= ~(1 << i);
	led_hwSetStatus(led, LED_OFF);
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
