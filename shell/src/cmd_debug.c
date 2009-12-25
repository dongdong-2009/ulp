/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

static time_t debug_deadline;
static int debug_counter;

static int cmd_debug_func(int argc, char *argv[])
{
	if(!debug_counter) {
		/*first time run*/
		debug_counter = 10;
		debug_deadline = time_get(1000);
		led_flash(LED_GREEN);
		return 1;
	}
	
	led_update();
	if(time_left(debug_deadline) < 0) {
		debug_deadline = time_get(1000);
		debug_counter --;
		if(debug_counter > 0)
			printf("\rdebug_ms_counter = %02d ", debug_counter);
		else {
			printf("debug command is over!!\n");
			return 0;
		}
	}
	return 1;
}
cmd_t cmd_debug = {"debug", cmd_debug_func, "for self-defined debug purpose"};
