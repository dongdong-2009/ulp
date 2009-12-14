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
	board_init();
	time_init();
	led_init();
	console_init();

	led_flash(LED_GREEN);
	printf("This is a zf103 board demo program\n");
	
  while (1) {
  	//module updates
  	led_update();
	
	if(console_IsNotEmpty()) {
		char str[64];
		scanf("%s", str);
		printf("str: %s, len=%d\n", str, strlen(str));
	}
  }
}
