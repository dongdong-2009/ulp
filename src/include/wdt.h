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

void wdt_update(void);        //feed the dog
int wdt_enable(void);
int wdt_disable(void);

#endif /*__WDT_H_*/
