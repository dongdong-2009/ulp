/*
 * 	miaofng@2010 initial version
 */
 
#ifndef __LM3S_H_
#define __LM3S_H_

#include "config.h"

#if CONFIG_CPU_LM3S8962 == 1
#include "inc/lm3s8962.h"
#endif

#include "lm3s_cmsis.h"

#if CONFIG_USE_TI_DRIVER_LIB == 1
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/can.h"
#include "driverlib/comp.h"
#include "driverlib/cpu.h"
#include "driverlib/debug.h"
#include "driverlib/epi.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/i2c.h"
#include "driverlib/i2s.h"
#include "driverlib/interrupt.h"
#include "driverlib/mpu.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/qei.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "driverlib/usb.h"
#include "driverlib/watchdog.h"
#endif

#endif
