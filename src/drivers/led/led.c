/* led.c
 * 	miaofng@2009 initial version
 *	- maxium 8 leds are supported
 */

#include "config.h"
#include "led.h"
#include "ulp_time.h"

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
	led_estep = 0;
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
		led_timer = time_get(LED_ERROR_PERIOD);
		if(led_estep < LED_ERROR_IDLE) {
			led_off(LED_RED);
		}
		else {
			led_inv(LED_RED);
		}
		led_estep ++;
		led_estep = (led_estep > (LED_ERROR_IDLE + 1 + 2 * (led_ecode - 1))) ? 0 : led_estep;
	}
#endif

	led_Update_Immediate();
}

void led_Update_Immediate(void)
{
	int led;
	int update;
	int status;

	/*calcu the new status of leds*/
	status = flag_hwstatus;
	status ^= flag_flash;
	status &= flag_flash;
	status |= flag_status;

	update = status ^ flag_hwstatus;
	flag_hwstatus = status;

	led = 0;
	while(update > 0) {
		if(update & 1) {
#ifdef CONFIG_LED_DUAL
			if((led_t) led == LED_YELLOW) {
				led_hwSetStatus(LED_RED, (led_status_t)(status & 1));
				led_hwSetStatus(LED_GREEN, (led_status_t)(status & 1));
			}
			else
#else
			led_hwSetStatus((led_t)led, (led_status_t)(status & 1));
#endif
		}

		led ++;
		update >>= 1;
		status >>= 1;
	}
}

void led_on(led_t led)
{
	int i = (int)led;
#ifdef CONFIG_LED_ECODE
	led_error(0);
#endif

#if CONFIG_LED_DUAL
	//clear red/green/yellow
	flag_status &= ~0x07;
	flag_flash &= ~0x07;
#endif

	flag_status |= 1 << i;
	flag_flash &= ~(1 << i);
	led_Update_Immediate();
}

void led_off(led_t led)
{
	int i = (int)led;

#ifdef CONFIG_LED_ECODE
	led_error(0);
#endif

#if CONFIG_LED_DUAL
	//clear red/green/yellow
	flag_status &= ~0x07;
	flag_flash &= ~0x07;
#endif

	flag_status &= ~(1 << i);
	flag_flash &= ~(1 << i);

	led_hwSetStatus(led, LED_OFF);
}

void led_inv(led_t led)
{
	int i = (int)led;

#ifdef CONFIG_LED_ECODE
	led_error(0);
#endif

#if CONFIG_LED_DUAL
	flag_status &= ~(0x07 & (~(1 << i)));
	flag_flash &= ~0x07;
#endif

	flag_status ^= 1 << i;
	flag_flash &= ~(1 << i);

	led_Update_Immediate();
}

void led_flash(led_t led)
{
	int i = (int)led;

#ifdef CONFIG_LED_ECODE
	led_error(0);
#endif

#if CONFIG_LED_DUAL
	flag_status &= ~(0x07 & (~(1 << i)));
	flag_flash &= ~0x07;
#endif

	flag_status &= ~(1 << i);
	flag_flash |= 1 << i;
}

#ifdef CONFIG_LED_ECODE
void led_error(int ecode)
{
	if(ecode) {
#if CONFIG_LED_DUAL
		flag_status &= ~0x07;
		flag_flash &= ~0x07;
#else
		flag_status &= ~0x02;
		flag_flash &= ~0x02;
#endif
	}

	ecode = (ecode < 0) ? - ecode : ecode;
	if((ecode == 0) || (led_ecode == 0)) {
		led_ecode = (char) ecode;
		led_estep = 0;
	}
}
#endif
