/*
*
* miaofng@2018-01-31 init version
*
*/

#include "led.h"
#include "gpio.h"

static int lsys = GPIO_NONE;
static int lerr = GPIO_NONE;

void led_hwSetStatus(led_t led, led_status_t status)
{
	if((lsys == GPIO_NONE) && (lerr == GPIO_NONE)){
		lsys = gpio_handle("LED_G");
		lerr = gpio_handle("LED_R");
	}

	int yes = (status == LED_ON) ? 1 : 0;
	switch(led) {
	case LED_SYS:
		gpio_set_h(lsys, yes);
		break;
	case LED_ERR:
		gpio_set_h(lerr, yes);
		break;
	default:
		break;
	}
}
