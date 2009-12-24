/* debug.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "board_debug.h"
#include <stdio.h>
#include <string.h>

void board_debug(void)
{
	int i = 0;
	time_t deadline;
	int volt=0;
	float temp=0;
	
	board_init();
	time_init();
	led_init();
	console_init();
	adc_init();
	pwm_init();
	pwmdebug_init();

	pwm_on();
	pwmdebug_on();

	printf("\n\nThis is hurry board test program\n");
	led_flash(LED_GREEN);
	deadline = time_get(1000);
	
  while (1) {
  	//module updates
  	led_update();
	time_update();
	
	//adc debug
	volt = adc_getvolt1();
	temp = adc_gettemp();

	//display per second
	if( time_left(deadline) < 0) {
		deadline = time_get(1000);
		i++;
		printf("\ri=%06d",i);
		//display temperature
		printf("  volt = %04dmv,temp=%03.1f", volt, temp);		
		
		pwmdebug_config(PWMDEBUG_CH4,1000,i);
		pwm_config(PWM_W,1000, i);
		if(i == 100)
			i=0;
	}
  }
}
