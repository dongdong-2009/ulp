/* time.c
 * 	miaofng@2009 initial version, be carefull of the overflow bug!!!
 */

#include "config.h"
#include "time.h"

void time_hwInit(void);

static time_t jiffies;

/*use RTC as timebase, tick period = 1ms*/
void time_Init(void)
{
	jiffies = 0;
	time_hwInit();
}

void time_Update(void)
{
}

void time_isr(void)
{
	jiffies ++;
}

/*unit: ms*/
time_t time_get(int delay)
{
	return (time_t)(jiffies + delay);
}

int time_left(time_t deadline)
{
	return (int)(deadline - jiffies);
}

void udelay(int us)
{
	while(us > 0) us--;
}

void mdelay(int ms)
{
	time_t deadline = time_get(ms);
	while(time_left(deadline) > 0);
}

void sdelay(int ss)
{
	time_t deadline = time_get(ss*1000);
	while(time_left(deadline) > 0);
}

#if CONFIG_CPU_STM32 == 1
#include "stm32f10x.h"
#elif CONFIG_CPU_LM3S == 1
#include "lm3s.h"
#endif

void time_hwInit(void)
{
	SysTick_Config(SystemFrequency / 1000);
}
