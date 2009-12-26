/* time.c
 * 	miaofng@2009 initial version, be carefull of the overflow bug!!!
 */

#include "config.h"
#include "stm32f10x.h"
#include "time.h"

/*use RTC as timebase, tick period = 1ms*/
void time_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	PWR_BackupAccessCmd(ENABLE);
#if 1
	RCC_LSICmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
#else
	RCC_LSEConfig(RCC_LSE_ON);
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#endif
	
	RCC_RTCCLKCmd(ENABLE);
	
	RTC_WaitForSynchro();
	RTC_WaitForLastTask();
	RTC_SetPrescaler(40000/1000 - 1); /*tick = 1Khz*/
	
	RTC_WaitForLastTask();
	RTC_SetCounter(0);
}

void time_update(void)
{
#if 0
	int val;
	RTC_WaitForLastTask();
	val = RTC_GetCounter();
#endif
}

/*unit: ms*/
time_t time_get(int delay)
{
	int val,deadline;
	
	RTC_WaitForLastTask();
	val = RTC_GetCounter();
	deadline = (unsigned)val + delay;
	
	return (time_t)deadline;
}

int time_left(time_t deadline)
{
	int val, left;
	
	RTC_WaitForLastTask();
	val = RTC_GetCounter();
	left = (int)deadline - val;
	
	return left;
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
