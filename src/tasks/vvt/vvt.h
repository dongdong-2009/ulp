/*
 * miaofng@2010 initial version
 * David@2010 initial version
 */
#ifndef __VVT_H_
#define __VVT_H_
#include "led.h"

#define WSS_MAX_RPM				20000
#define VSS_MAX_RPM				10000
#define KNOCK_MAX_FRQ			20000	//20k
#define NE58X_MAX_RPM			5000
#define MISFIRE_MAX_STRENGTH	0x8000;

typedef enum {
KS1 = 0,
KS2,
KS3,
KS4,
NR_OF_KS
} knock_ch_t;

typedef enum {
NE58X = 0,
CAM1X,
CAM4X_IN,
CAM4X_EXT,
PSS_CH_NR
} pss_ch_t;

#define vvt_Start() do{ pss_SetSpeed(misfire_GetSpeed(0)); pss_Enable(1);led_on(LED_RED);} while(0)
#define vvt_Stop() do{pss_Enable(0);led_on(LED_RED);led_off(LED_GREEN);} while(0)

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
	(((val) >= (min)) && ((val) <= (max)))

void vvt_Init(void);
void vvt_Update(void);
void vvt_isr(void);
#endif /*__VVT_H_*/

