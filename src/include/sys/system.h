/*
 * 	miaofng@2009 initial version
 */
#ifndef __SYSTEM_H_
#define __SYSTEM_H_

void sys_Init(void);
void sys_Update(void);

/*normally this routine will be provided by cpu/xxx/cmsis/system_xxx.c*/
extern void SystemInit (void);

#include "ulp_time.h"
#endif /*__SYSTEM_H_*/
