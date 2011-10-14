/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "cncb.h"
#include "nest_light.h"
#include "nest_core.h"
#include "ulp_time.h"

static time_t nest_timer_toggle = 0;

int nest_light(int cmd)
{
	if(cmd >= FAIL_ABORT_TOGGLE) {
		nest_update();
		if(time_left(nest_timer_toggle) < 0) {
			nest_timer_toggle = time_get(500);
		}
		else {
			return 1;
		}
	}

	switch (cmd) {
	case FAIL_ABORT_ON:
		cncb_signal(LED_F, LSD_ON);
		break;
	case FAIL_ABORT_OFF:
		cncb_signal(LED_F, LSD_OFF);
		break;
	case FAIL_ABORT_TOGGLE:
		cncb_signal(LED_F, TOGGLE);
		break;
	case PASS_CMPLT_ON:
		cncb_signal(LED_P, LSD_ON);
		break;
	case PASS_CMPLT_OFF:
		cncb_signal(LED_P, LSD_OFF);
		break;
	case PASS_CMPLT_TOGGLE:
		cncb_signal(LED_P, TOGGLE);
		break;
	case RUNNING_ON:
		cncb_signal(LED_R, LSD_ON);
		break;
	case RUNNING_OFF:
		cncb_signal(LED_R, LSD_OFF);
		break;
	case RUNNING_TOGGLE:
		cncb_signal(LED_R, TOGGLE);
		break;
	case ALL_ON:
		cncb_signal(LED_F, LSD_ON);
		cncb_signal(LED_P, LSD_ON);
		cncb_signal(LED_R, LSD_ON);
		break;
	case ALL_OFF:
		cncb_signal(LED_F, LSD_OFF);
		cncb_signal(LED_P, LSD_OFF);
		cncb_signal(LED_R, LSD_OFF);
		break;
	case ALL_TOGGLE:
		cncb_signal(LED_F, TOGGLE);
		cncb_signal(LED_P, TOGGLE);
		cncb_signal(LED_R, TOGGLE);
		break;
	default:;
	}
	return 0;
}

int nest_light_flash(int err_code)
{
	for(; err_code > 0; err_code --) {
		//turn on
		nest_light(RUNNING_ON) ;
		nest_mdelay(300);

		//turn off
		nest_light(RUNNING_OFF);
		nest_mdelay(300);
	}

	nest_mdelay(1000);
	return 0;
}
