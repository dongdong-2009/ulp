/* debug.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "board_debug.h"

void board_debug(void)
{
	board_init();
	time_init();
	led_init();
	console_init();

	led_flash(LED_GREEN);
	
  while (1) {
  	//module updates
  	led_update();

	//misc
	console_putchar('T');
	mdelay(250);
	console_putchar('x');
	mdelay(250);
  }
}
