/* time.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stm32f10x_lib.h"
#include "time.h"

void time_init(void)
{
}

#if 1
void Delay(vu32 nCount)
{
	for(; nCount != 0; nCount--);
}
#endif

void sdelay(unsigned int ss)
{
	while(ss>0) {
		ss --;
		Delay(0xAFFFF);
	}
}
