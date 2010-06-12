/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "led.h"
#include "time.h"

void led_hwInit(void)
{
	/*zlg develope board:
		LED5	PA4
		LED6	PA5
	*/
	SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA;
	udelay(10);
	GPIO_PORTA_DIR_R |= (1 << 4) | (1 << 5);
	GPIO_PORTA_DEN_R |= (1 << 4) | (1 << 5);
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	int ba;
	switch(status) {
		case LED_OFF:
			ba = 1;
			break;
		case LED_ON:
			ba = 0;
			break;
		default:
			return;
	}
	
	switch(led) {
		case LED_GREEN:
			GPIO_PORTA_DATA_R &= ~(1 << 4);
			GPIO_PORTA_DATA_R |= ba << 4;
			break;
		case LED_RED:
			GPIO_PORTA_DATA_R &= ~(1 << 5);
			GPIO_PORTA_DATA_R |= ba << 5;
			break;
		default:
			break;
	}
}
