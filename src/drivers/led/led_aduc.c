/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "led.h"
#include "aduc706x.h"

void led_hwInit(void)
{
	//led_r p1.6 low->on
	GP1DAT |= 1 << (24 + 6); //DIR = OUT
	GP1SET = 1 << (16 + 6); //led off
	GP1CLR = 1 << (16 + 6); //led on
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	switch(led) {
		case LED_GREEN:
			break;
		case LED_RED:
			GP1SET = (LED_ON) ? (1 << (16 + 6)) : 0;
			GP1CLR = (LED_OFF) ? (1 << (16 + 6)) : 0;
			break;
		case LED_YELLOW:
			break;
		default:
			break;
	}
}
