/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "nest.h"
#include "sys/task.h"
#include "common/circbuf.h"
#include "nvm.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/sys.h>
#include <shell/cmd.h>
#include "debug.h"

static time_t nest_timer;
static time_t nest_timer_toggle;
static struct nest_error_s nest_err;
static struct nest_info_s nest_info;
#if CONFIG_NEST_LOG_SIZE > 0
static char nest_log_buf[CONFIG_NEST_LOG_SIZE] __nvm;
static circbuf_t nest_log;
#endif

int nest_init(void)
{
	nest_timer_toggle = 0;
	task_Init();
	cncb_init();
	nest_message_init();
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
#if CONFIG_NEST_LOG_SIZE > 0
	//save the log message to flash
	nvm_save();
#endif
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

int nest_message_init(void)
{
#if CONFIG_NEST_LOG_SIZE > 0
	nest_log.data = nest_log_buf;
	buf_init(&nest_log, CONFIG_NEST_LOG_SIZE);
#endif
	return 0;
}

int nest_message(const char *fmt, ...)
{
	va_list ap;
	char *pstr;
	int n = 0;

	pstr = (char *) sys_malloc(256);
	assert(pstr != NULL);
	va_start(ap, fmt);
	n += vsnprintf(pstr + n, 256 - n, fmt, ap);
	va_end(ap);

	//output string to console or log buffer
	printf("%s", pstr);
#if CONFIG_NEST_LOG_SIZE > 0
	buf_push(&nest_log, pstr, n);
#endif
	sys_free(pstr);
	return 0;
}

//nest shell command
static int cmd_nest_func(int argc, char *argv[])
{
	const char *usage = {
		"nest help\n"
#if CONFIG_NEST_LOG_SIZE > 0
		"nest log	print log message\n"
#endif
	};

	if(argc > 1) {
#if CONFIG_NEST_LOG_SIZE > 0
		if(!strcmp(argv[1], "log")) {
			int n;
			circbuf_t tmpbuf; //for view only purpose
			memcpy(&tmpbuf, &nest_log, sizeof(circbuf_t));
			char *pstr = sys_malloc(256);
			assert(pstr != NULL);
			do {
				n = buf_pop(&tmpbuf, pstr, 256);
				pstr[n] = 0;
				printf("%s", pstr);
			} while(n != 0);
			sys_free(pstr);
			return 0;
		}
#endif
	}
	printf("%s", usage);
	return 0;
}

const static cmd_t cmd_nest = {"nest", cmd_nest_func, "nest debug command"};
DECLARE_SHELL_CMD(cmd_nest)
