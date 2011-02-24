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
