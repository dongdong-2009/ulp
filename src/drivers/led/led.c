/* led.c
 * 	miaofng@2009 initial version
 *	- maxium 8 leds are supported
 */

#include "config.h"
#include "led.h"
#include "ulp_time.h"

#define CONFIG_LED_ECODE 1

static time_t led_timer;
static char flag_status; /*0->off/flash, 1->on*/
static char flag_flash; /*1->flash*/
static char flag_hwstatus; /*owned by led_update thread only*/

#ifdef CONFIG_LED_ECODE
static char led_ecode;
static char led_estep; /*0/1: idle, other->counting ..*/
#endif

void led_Init(void)
{
	led_timer = 0;
	flag_status = 0;
	flag_flash = 0;
	flag_hwstatus = 0;
#ifdef CONFIG_LED_ECODE
	led_ecode = 0;
#endif

	led_hwInit();
	led_flash(LED_GREEN);
}

void led_Update(void)
{
	int led;
	int update;
	int status;

	if(time_left(led_timer) > 0)
		return;

	led_timer = time_get(LED_FLASH_PERIOD);

#ifdef CONFIG_LED_ECODE
	if(led_ecode != 0) { //ecode=2: 1110 10 10
		if(led_estep < LED_IDLE_PERIOD) {
			led_on(LED_RED);
		}
		else {
			led_inv(LED_RED);
		}
		led_estep ++;
		led_estep = (led_estep > (LED_IDLE_PERIOD + 1 + 2 * led_ecode)) ? 0 : led_estep;
	}
#endif

	/*calcu the new status of leds*/
	status = flag_hwstatus;
	status ^= flag_flash;
	status &= flag_flash;
	status |= flag_status;

	update = status ^ flag_hwstatus;
	flag_hwstatus = status;

	led = 0;
	while(update > 0) {
		if(update & 1)
			led_hwSetStatus((led_t)led, (led_status_t)(status & 1));

		led ++;
		update >>= 1;
		status >>= 1;
	}
}

void led_Update_Immediate(void)
{
	int led;
	int update;
	int status;

	led_timer = time_get(LED_FLASH_PERIOD);

	/*calcu the new status of leds*/
	status = flag_hwstatus;
	status ^= flag_flash;
	status &= flag_flash;
	status |= flag_status;

	update = status ^ flag_hwstatus;
	flag_hwstatus = status;

	led = 0;
	while(update > 0) {
		if(update & 1)
			led_hwSetStatus((led_t)led, (led_status_t)(status & 1));

		led ++;
		update >>= 1;
		status >>= 1;
	}
}

void led_on(led_t led)
{
	int i = (int)led;

	flag_status |= 1 << i;
	flag_flash &= ~(1 << i);
	led_hwSetStatus(led, LED_ON);
}

void led_off(led_t led)
{
	int i = (int)led;

	flag_status &= ~(1 << i);
	flag_flash &= ~(1 << i);
	led_hwSetStatus(led, LED_OFF);
}

void led_inv(led_t led)
{
	int i = (int)led;

	flag_status ^= 1 << i;
	flag_flash &= ~(1 << i);
	led_hwSetStatus(led, (flag_status & (1 << i)) ? LED_ON : LED_OFF);
}

void led_flash(led_t led)
{
	int i = (int)led;

	flag_status &= ~(1 << i);
	flag_flash |= 1 << i;
}

void led_error(char ecode)
{
	if((ecode == 0) || (led_ecode == 0)) {
		led_ecode = ecode;
		led_estep = 0;
	}
}
