/*
 * 	miaofng@2013-2-21 initial version of multi-channel DMM(MCD)
 *	- input matrix control > MBI5025
 *	- dmm i/f > ADUC706X
 *	- sys_update() will be called during longer wait period(>1mS)
 */

#ifndef __OID_MCD_H__
#define __OID_MCD_H__

enum {
	DMM_MODE_R,
	DMM_MODE_V,
};

int mcd_init(void);
int mcd_mode(char mode);
int mcd_read(int *value);
int mcd_pick(int pin0, int pin1); //pin0: 0..5, pin1: 0..5 (pin0 != pin1)

/*reserved for ybs monitor usage*/
int mcd_xread(int ch, int *value); //ch = 0 .. 11
void mcd_relay(int image);

#endif
