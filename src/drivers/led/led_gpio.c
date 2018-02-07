/*
*
* miaofng@2018-01-31 init version
*
*/

#include "led.h"
#include "gpio.h"

int gpio_led_g = GPIO_INVALID;
int gpio_led_r = GPIO_INVALID;

void led_hwSetStatus(led_t led, led_status_t status)
{
	if(gpio_led_g == GPIO_INVALID) gpio_led_g = gpio_handle("LED_G");
	if(gpio_led_r == GPIO_INVALID) gpio_led_r = gpio_handle("LED_R");

	int yes = (status == LED_ON) ? 1 : 0;
	switch(led) {
	case LED_GREEN:
		if(gpio_led_g != GPIO_NONE) gpio_set_h(gpio_led_g, yes);
		break;
	case LED_RED:
		if(gpio_led_r != GPIO_NONE) gpio_set_h(gpio_led_r, yes);
		break;
	default:
		break;
	}
}
