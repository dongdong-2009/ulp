/*
 * 	david@2012 initial version
 *
 */
#ifndef __WDT_H_
#define __WDT_H_

#include "config.h"

//init the watch dog reset period(ms) time
//the range of watch dog update time is 1ms - 3000ms
int wdt_init(int period);

//feed the dog
void wdt_update(void);

#endif /*__WDT_H_*/
