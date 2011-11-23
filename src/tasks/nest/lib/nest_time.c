/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "ulp_time.h"
#include "nest_time.h"

static time_t nest_time_start;

void nest_time_init(void)
{
	nest_time_start = time_get(0);
}

int nest_time_get(int mdelay)
{
	return time_get(mdelay) - nest_time_start;
}

int nest_time_left(int deadline)
{
	return time_left(deadline + nest_time_start);
}

