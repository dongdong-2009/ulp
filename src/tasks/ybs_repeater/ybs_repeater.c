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
#include "ybs_repeater.h"
#include "ybs_dc.h"
#include "ybs_dsp.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_rtc.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"
#include "ybs_sensor.h"
#include "lpc177x_8x.h"

ybs_sensor_t sensor[SENSOR_NUMBER];
static uint32_t systick_enable = 0;
int sensor_now = 0;
unsigned int address;
int average_point_number = 2;
int alarm_cnt_max = 100;
int cut_cnt_max = 100;
int no_response_cnt_max = 100;

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
	if(!systick_enable) {
		return;
	}
	sensor_now++;
	sensor_now = sensor_now > 9 ? 0 : sensor_now;
	if(sensor_now < (SENSOR_NUMBER >> 1)) {
		unsigned short temp1;
		unsigned short temp2;
		int temp3 = (sensor_now + 1) < (SENSOR_NUMBER >> 1) ? sensor_now + 1 : 0;

		read_sensor(&temp1, &temp2);
		write_sensor(temp3, 'R');

		dc_config_ybs(sensor_now);
		temp1 = ybs_dsp(sensor_now, temp1);
		dc_save(temp1);

		dc_config_ybs(sensor_now + (SENSOR_NUMBER >> 1));
		temp2 = ybs_dsp(sensor_now + (SENSOR_NUMBER >> 1), temp2);
		dc_save(temp2);
	}
}

void app_init()
{
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);		//enable gpio clk
	rtc_init();
	address_init();
	address = address_read();
	sensor_init();
	systick_enable = 1;
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
