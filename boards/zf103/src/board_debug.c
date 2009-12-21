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
	int volt, temp, freq, duty = 50;
	
	board_init();
	time_init();
	led_init();
	console_init();
	adc_init();
	pwm_init();

	/*clear screen*/
	/*ref: http://www.cnblogs.com/mugua/archive/2009/11/25/1610118.html*/
	putchar(0x1b);
	putchar('c');
	printf("This is a zf103 board demo program\n");
	pwm_on();
	led_flash(LED_GREEN);
	
  while (1) {
  	//module updates
  	led_update();
	
	//adc debug
	volt = adc_getvolt();
	temp = adc_gettemp();

	if(duty <0 ) duty = 0;
	if(duty > 100) duty = 100;
	
	freq = volt*4; /*3300*4 = 13,200Hz*/
	pwm_config(freq, duty);

	//get duty factor from console
	if(console_IsNotEmpty()) {
		char c = (char)console_getchar();
		if(c == 'w') duty +=10;
		else duty -= 10;
	}

	//display
	printf("\rvolt = %04dmv,temp=%04d, freq = %05dHz, duty=%02d", volt, temp, freq, duty);
  }
}
