/* led.c
 * 	miaofng@2009 initial version
 *	- maxium 8 leds are supported
 */

#include "config.h"
#include "led.h"
#include "time.h"

static time_t led_timer;
static char led_status; /*0->off/flash, 1->on*/
static char led_flash; /*1->flash*/
static char led_hwstatus; /*owned by led_update thread only*/

void led_Init(void)
{
	led_timer = 0;
	led_status = 0;
	led_flash = 0;
	led_hwstatus = 0;
	
	led_hwInit();
}

void led_Update(void)
{
	int led;
	int update;
	int status;

	if(time_left(led_timer) > 0)
		return;
	
	led_timer = time_get(LED_FLASH_PERIOD);
	
	/*calcu the new status of leds*/
	status = led_hwstatus;
	status ^= led_flash;
	status &= led_flash;
	status |= led_status;

	update = status ^ led_hwstatus;
	led_hwstatus = status;

	led = 0;
	while(update > 0) {
		if(update & 1)
			led_hwSetStatus(led, status & 1);

		led ++;
		update >>= 1;
		status >>= 1;
	}
}

void led_on(led_t led)
{
	int i = (int)led;
	
	led_status |= 1 << i;
	led_flash &= ~(1 << i);
}

void led_off(led_t led)
{
	int i = (int)led;
	
	led_status &= ~(1 << i);
	led_flash &= ~(1 << i);
}

void led_inv(led_t led)
{
	int i = (int)led;
	
	led_status ^= 1 << i;
	led_flash &= ~(1 << i);
}

void led_flash(led_t led)
{
	int i = (int)led;
	
	led_status &= ~(1 << i);
	led_flash |= 1 << i;
}
