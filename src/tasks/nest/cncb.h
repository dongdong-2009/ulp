/*
 * 	miaofng@2010 initial version
 */

#ifndef __CNCB_H_
#define __CNCB_H_

#include "config.h"

/*signal def*/
enum {
	/*Digital Output Pins*/
	SIG1,
	SIG2,
	SIG3,
	SIG4,
	SIG5,
	SIG6,

	/*Low Side Drive Pins*/
	BAT,
	IGN,
	LSD,
	LED_F,
	LED_R,
	LED_P,

	/*misc ctrl*/
	CAN_MD0,
	CAN_MD1,
	CAN_SEL,
};

/*signal status*/
enum {
	//SIGn high/low level
	SIG_LO = 0,
	SIG_HI = 1,

	//LSDn float/drive
	LSD_OFF = SIG_LO,
	LSD_ON = SIG_HI,

	//common ops
	TOGGLE,
};

/*cncb api*/
int cncb_init(void);
int cncb_detect(int debounce);
int cncb_signal(int sig, int level);

#endif
