/* debug.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x_lib.h"
#include "debug.h"
#include "led.h"
#include "console.h"
#include "time.h"

void board_debug(void)
{
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