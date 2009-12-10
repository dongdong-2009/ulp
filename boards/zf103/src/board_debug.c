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
	
  while (1) {
	/*turn on led*/
	console_putchar('T');
	led_on();
	sdelay(1);

	/* Turn off led */
	console_putchar('x');
	led_off();
	sdelay(1);
  }
}
