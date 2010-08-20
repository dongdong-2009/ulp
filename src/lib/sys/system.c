/* board.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sys/system.h"
#include "led.h"
#include "console.h"
#include "spi.h"

void sys_Init(void)
{
	
	SystemInit();
	time_Init();
	led_Init();
#if (CONFIG_IAR_REDIRECT == 1) || (CONFIG_TASK_SHELL == 1)
	console_Init();
#endif
}

void sys_Update(void)
{
	led_Update();
	time_Update();
}
