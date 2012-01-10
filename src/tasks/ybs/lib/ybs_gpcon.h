/*
 * 	junjun@2011 initial version
 */

#ifndef __YBS_GPCON_H_
#define __YBS_GPCON_H_

/*signal def*/
enum {
	/*Digital Output Pins*/
	SENSOR,
};

/*Simulate film stress */
enum {
	//SIGn high/low level
	SIG_LO = 0,
	SIG_HI = 1,

	//sensor pressed ON/OFF
	SENPR_OFF = SIG_LO,
	SENPR_ON = SIG_HI,

	//common ops
	TOGGLE,
};

/*cncb api*/
int gpcon_init(void);
int gpcon_signal(int sig, int ops);

#endif
