/*
 *	miaofng@2009 initial version, be carefull of the overflow bug!!!
 * 	miaofng@2012-12-30 modified to rtc driver
 */

#include "config.h"
#include "ulp_time.h"
#include "stm32f10x.h"

void rtc_init(unsigned now)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	PWR_BackupAccessCmd(ENABLE);
#if 0
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
	RTC_SetPrescaler(CONFIG_LSE_VALUE/1 - 1); /*tick = 1Hz*/

	RTC_WaitForLastTask();
	RTC_SetCounter(now);
	PWR_BackupAccessCmd(DISABLE);
}

unsigned rtc_get(void)
{
	RTC_WaitForLastTask();
	return RTC_GetCounter();
}

void rtc_alarm(unsigned t)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	PWR_BackupAccessCmd(ENABLE);
	RTC_WaitForLastTask();
	RTC_SetAlarm(t);
	PWR_BackupAccessCmd(DISABLE);
}
