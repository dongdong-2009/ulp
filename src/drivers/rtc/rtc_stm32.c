/*
 *	miaofng@2009 initial version, be carefull of the overflow bug!!!
 * 	miaofng@2012-12-30 modified to rtc driver
 *	miaofng@2013-1-19 note:
 *	- both rtc and bkp are in backup (power) domain(powered by VBAT)
 *	- rtc is in 32KHz clock domain, so you must RTC_WaitForLastTask() before
 *	setting RTC_PRL/DIV/CNT/ALR
 *	- bkp is in APB1 clock domain, so it's similar as other register
 *	except its power is from VBAT.
 *	- bkp registers has write protection function
 */

#include "config.h"
#include "ulp_time.h"
#include "stm32f10x.h"

void rtc_init(unsigned now)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,ENABLE);
	PWR_BackupAccessCmd(ENABLE); //disable bkp domain write protection

	/*note: once the rtc clock source has been set, it cannot be changed
	anymore unless the bkp domain is reset!!!*/
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
	RTC_WaitForLastTask();
	PWR_BackupAccessCmd(DISABLE);
}

unsigned rtc_get(void)
{
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	//PWR_BackupAccessCmd(ENABLE);
	//RTC_WaitForLastTask();
	unsigned seconds = RTC_GetCounter();
	//PWR_BackupAccessCmd(DISABLE);
	return seconds;
}

void rtc_alarm(unsigned t)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	PWR_BackupAccessCmd(ENABLE);
	RTC_WaitForLastTask();
	RTC_SetAlarm(t);
	PWR_BackupAccessCmd(DISABLE);
}
