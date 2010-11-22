/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "nest.h"
#include "sys/task.h"

static time_t nest_timer;
static time_t nest_timer_toggle;
static struct nest_error_s nest_err;
static struct nest_info_s nest_info;

int nest_init(void)
{
	nest_timer_toggle = 0;
	task_Init();
	cncb_init();
	return 0;
}

int nest_update(void)
{
	task_Update();
	return 0;
}

void nest_time_init(void)
{
	nest_timer = time_get(0);
}

int nest_time_get(void)
{
	return - time_left(nest_timer);
}

int nest_error_init(void)
{
	nest_err.type = NO_FAIL;
	nest_err.time = 0;
	nest_err.info = "Complete";
	return 0;
}

struct nest_error_s* nest_error_get(void)
{
	return &nest_err;
}

int nest_error_set(int type, const char *info)
{
	nest_err.type = type;
	nest_err.time = nest_time_get();
	nest_err.info = info;
	nest_message("%s fail(error=%d,time=%d)!!!\n", info, type, nest_err.time);
	return 0;
}

int nest_light(int cmd)
{
	if(cmd >= FAIL_ABORT_TOGGLE) {
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

int nest_light_flash(int code)
{
	for(; code > 0; code --) {
		//turn on
		nest_light(RUNNING_ON) ;
		mdelay(300);

		//turn off
		nest_light(RUNNING_OFF);
		mdelay(300);
	}

	mdelay(1000);
	return 0;
}

int nest_can_sel(int ch)
{
	if(ch == DW_CAN) {
		cncb_signal(CAN_SEL, SIG_HI);
		return 0;
	}

	if(ch == SW_CAN) { //md0, md1 00->sleep, 01->wake, 10->tx high speed, 11 -> normal speed&voltage
		cncb_signal(CAN_SEL, SIG_LO);
		cncb_signal(CAN_MD0, SIG_HI);
		cncb_signal(CAN_MD1, SIG_HI);
		return 0;
	}

	return -1;
}

struct nest_info_s* nest_info_get(void)
{
	return &nest_info;
}

