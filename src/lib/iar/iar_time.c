/*******************
 *
 * Copyright 1998-2010 IAR Systems AB. 
 *
 * This is the default implementation of the "time" function of the
 * standard library.  It can be replaced with a system-specific
 * implementation.
 *
 * The "time" function returns the current calendar time.  (time_t)-1
 * should be returned if the calendar time is not available.  The time
 * is measured in seconds since the first of January 1970.
 *
 ********************/

#include "config.h"
#include "ulp_time.h"

#pragma module_name = "?time"

__time32_t (__time32)(__time32_t *t)
{
	unsigned now = -1;
#ifdef CONFIG_DRIVER_RTC
	now = rtc_get();
#endif
	if (t) {
		*t = (__time32_t) now;
	}
	return (__time32_t) now;
}
