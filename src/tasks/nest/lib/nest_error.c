/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "cncb.h"
#include "nest_error.h"
#include "nest_message.h"
#include "nest_time.h"
#include "nvm.h"

static struct nest_error_s nest_err;

int nest_error_init(void)
{
	nest_err.type = NO_FAIL;
	nest_err.time = 0;
	nest_err.info = "Complete";
	nest_err.nec = 0;
	return 0;
}

struct nest_error_s* nest_error_get(void)
{
	return &nest_err;
}

int nest_error_set(int type, const char *info)
{
	int min;
	
	nest_err.type = type;
	nest_err.time = nest_time_get(0);
	nest_err.info = info;
	nest_message("%s fail(error=%d,time=%dms)!!!\n", info, type, nest_err.time);
	
	min = nest_err.time / 1000 / 60;
	min = min > 15 ? 15 : 0;
	nest_err.nec = (min << 4) | (type & 0x0f);
#if CONFIG_NEST_LOG_SIZE > 0
	//save the log message to flash
	nvm_save();
#endif
	return 0;
}
