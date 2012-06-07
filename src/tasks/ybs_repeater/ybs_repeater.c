/*
 * 	king@2012 initial version
 */

#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "sys/task.h"
#include "ulp/sys.h"
#include "shell/cmd.h"
#include <stdlib.h>
#include "ybs_sensor.h"

extern ybs_sensor_t sensor[SENSORNUMBER];
static uint32_t sensor_enable = 0;

/*
 * 	use USB clock to drive ethernet
 */
void ethernet_clk_init()
{
	PINSEL_ConfigPin(1,27,4);
	LPC_SC->PLL1CON = 0x00;			/* PLL1 Disable */
	LPC_SC->PLL1CFG = 24;
	LPC_SC->PLL1CON = 0x01;			/* PLL1 Enable */
	LPC_SC->PLL1FEED = 0xAA;
	LPC_SC->PLL1FEED = 0x55;
	while (!(LPC_SC->PLL1STAT & (1<<10)));		/* Wait for PLOCK1 */
	LPC_SC->USBCLKSEL = (0x00000006|(0x02<<8)); /* Setup USB Clock Divider */
	LPC_SC->CLKOUTCFG = 3 | 0x00000100;			//set USB clock as clkout
}


void address_init()
{
	GPIO_SetDir(2, ADDRESS, 0);		//set p2[0-7] as input
}

uint32_t address_read()
{
	return (0x000000ff & GPIO_ReadValue(2));
}

void rtc_init()
{
	RTC_Init(LPC_RTC);
	RTC_ResetClockTickCounter(LPC_RTC);
	RTC_Cmd(LPC_RTC, ENABLE);
}

void rtc_set(RTC_TIME_Type *time)
{
	LPC_RTC->SEC = time->SEC & RTC_SEC_MASK;
	LPC_RTC->MIN = time->MIN & RTC_MIN_MASK;
	LPC_RTC->HOUR = time->HOUR & RTC_HOUR_MASK;
	LPC_RTC->DOM = time->DOM & RTC_DOM_MASK;
	LPC_RTC->MONTH = time->MONTH & RTC_MONTH_MASK;
	LPC_RTC->YEAR = time->YEAR & RTC_YEAR_MASK;
}

void sys_tick()
{
	static uint32_t com_now = 0;
	if(!sensor_enable) {
		return;
	}
	com_now = com_now > 9 ? 0 : com_now;
	if(com_now < (SENSORNUMBER >> 1)) {
		uint32_t temp1 = sensor[com_now].normaldata.data_now & ~(0xffffffff << NORMALDATASIZESHIFT);
		uint32_t temp2 = sensor[com_now + (SENSORNUMBER >> 1)].normaldata.data_now & ~(0xffffffff << NORMALDATASIZESHIFT);
		read_sensor(com_now, &(sensor[com_now].normaldata.data[temp1]), &(sensor[com_now + (SENSORNUMBER >> 1)].normaldata.data[temp2]));
		uint32_t temp3 = com_now == ((SENSORNUMBER >> 1) - 1) ? 0 : com_now + 1;
		write_sensor(temp3, 'a');
		analize_sensor(com_now, sensor[com_now].normaldata.data[temp1]);
		analize_sensor(com_now + (SENSORNUMBER >> 1), sensor[com_now + (SENSORNUMBER >> 1)].normaldata.data[temp2]);
		sensor[com_now].normaldata.data_now++;
		sensor[com_now + (SENSORNUMBER >> 1)].normaldata.data_now++;
	}
	com_now++;
}


static uint32_t address;

void RTC_IRQHandler(void)
{
	RTC_TIME_Type time;

	/* This is increment counter interrupt*/
	if (RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE))
	{
		RTC_GetFullTime (LPC_RTC, &time);
		printf("%d-%02d-%02d %02d:%02d:%02d\n", time.YEAR,time.MONTH,time.DOM,time.HOUR,time.MIN,time.SEC);
		// Clear pending interrupt
		RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
	}
}

void app_init()
{

	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);		//enable gpio clk
	ethernet_clk_init();
	address_init();
	address = address_read();
	rtc_init();
	// RTC_TIME_Type time;
	// time.SEC = 1;
	// time.MIN = 1;
	// time.HOUR = 1;
	// time.DOM = 1;
	// time.MONTH = 1;
	// time.YEAR = 1;
	// rtc_set(&time);
	// RTC_CntIncrIntConfig (LPC_RTC, RTC_TIMETYPE_SECOND, ENABLE);
	// NVIC_EnableIRQ(RTC_IRQn);
	sensor_init();
	sensor_enable = 1;
	//sensor[0].status = 3;
}

void app_update()
{
	sensor_update();
}

int main(void)
{
	task_Init();
	app_init();
	for(;;) {
		task_Update();
		app_update();
	}
}
