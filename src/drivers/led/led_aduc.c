/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "led.h"
#include "aduc706x.h"

void led_hwInit(void)
{
#if defined(CONFIG_TASK_OID) || defined(CONFIG_TASK_YBS)
	//led_r p0.4 low->on
	GP0DAT |= 1 << (24 + 4); //DIR = OUT
	GP0SET = 1 << (16 + 4); //led off
	GP0CLR = 1 << (16 + 4); //led on
#else
	//led_r p1.6 low->on
	GP1DAT |= 1 << (24 + 6); //DIR = OUT
	GP1SET = 1 << (16 + 6); //led off
	GP1CLR = 1 << (16 + 6); //led on
#endif
	//led_flash(LED_RED);
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	switch(led) {
		case LED_GREEN:
			break;
		case LED_RED:
#if defined(CONFIG_TASK_OID) || defined(CONFIG_TASK_YBS)
			GP0CLR = (status == LED_ON) ? (1 << (16 + 4)) : 0;
			GP0SET = (status == LED_OFF) ? (1 << (16 + 4)) : 0;
#else
			GP1CLR = (status == LED_ON) ? (1 << (16 + 6)) : 0;
			GP1SET = (status == LED_OFF) ? (1 << (16 + 6)) : 0;
#endif
			break;
		case LED_YELLOW:
			break;
		default:
			break;
	}
}
