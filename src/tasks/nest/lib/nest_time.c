/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "time.h"
#include "nest_time.h"

static time_t nest_timer;

void nest_time_init(void)
{
	nest_timer = time_get(0);
}

int nest_time_get(void)
{
	return - time_left(nest_timer);
}

