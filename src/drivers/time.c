/* time.c
 * 	miaofng@2009 initial version, be carefull of the overflow bug!!!
 */

#include "config.h"
#include "stm32f10x.h"
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

#if CONFIG_CPU_STM32
void time_hwInit(void)
{
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000); /*1000Hz, 1ms per tick*/

	/* Update the SysTick IRQ priority should be higher than the Ethernet IRQ */
	/* The Localtime should be updated during the Ethernet packets processing */
	NVIC_SetPriority (SysTick_IRQn, 1);

	/*RTC init*/
#if 0
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
#endif
}
#endif