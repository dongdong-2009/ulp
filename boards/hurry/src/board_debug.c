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
	
	board_init();
	time_init();
	led_init();
	console_init();

	printf("\n\nThis is hurry board test program\n");
	led_flash(LED_GREEN);
	deadline = time_get(1000);
	
  while (1) {
  	//module updates
  	led_update();
	time_update();
	
	//display per second
	if( time_left(deadline) < 0) {
		deadline = time_get(1000);
		i++;
		printf("\ri=%06d",i);
	}
  }
}
