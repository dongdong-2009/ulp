/*
 * 	king@2012 initial version
 */

#ifndef __YBS_REPEATER_H_
#define __YBS_REPEATER_H_
#include <stdint.h>
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_rtc.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_pinsel.h"
#include "ybs_sensor.h"
#include "lpc177x_8x.h"

#define ADDRESS 0x000000ff		//p2[0-7]		the number of the board
#define MUX_CHANNEL 0x001c0000		//p1[18-20]		multiplexer channel-choice pin
#define MUX_ENABLE 0x00200000		//p1[21]		multiplexer enable


#define TIMEOFBEGIN 1000		//ms
#define TIMEOFERROR 1000		//ms



#endif /*__YBS_REPEATER_H_*/