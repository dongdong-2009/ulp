/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "driver.h"

void drv_Init(void)
{
	void (**init)(void);
	void (**end)(void);
	
	init = __section_begin(".driver");
	end = __section_end(".driver");
	while(init < end) {
		(*init)();
		init ++;
	}
}
