/* debug.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "board_debug.h"
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

void board_debug(void)
{
	int i = 0;
	int flag = 1;
	time_t deadline;
	
	board_Init();
	time_Init();
	led_Init();
	console_Init();	
	pwm_Init();
	pwmdebug_Init();
	pwm_On();
	pwmdebug_On();	

	printf("\n\nThis is hurry board test program\n");
	led_flash(LED_GREEN);
	led_flash(LED_YELLOW);
	deadline = time_get(10);
	
  while (1) {
  	//module updates
  	led_update();
	time_update();

	//display per second
	if( time_left(deadline) < 0) {
		deadline = time_get(10);//fresh 10 ms
		
		if(flag){
			i += 2;
			if(i >= 100)
				flag = 0;
		}
		else{
			i -= 2;
			if(i <=0 )
				flag = 1;
		}
		
		pwm_Config(PWM_U,i,REP_RATE);
		pwm_Config(PWM_V,i,REP_RATE);
		pwm_Config(PWM_W,i,REP_RATE);
		pwmdebug_Config(PWMDEBUG_CH4,i);
	}
  }
}
