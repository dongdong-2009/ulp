/*
 * 	miaofng@2013 initial version
 *	risk note:
 *	- bank switch need an settling time dueto relay factor & auto range search
 */

#ifndef __OID_DMM_H__
#define __OID_DMM_H__

#define _mohm(x) (x)
#define _ohm(x) (_mohm(x) * 1000)
#define _kohm(x) (_ohm(x) * 1000)

#define _uv(x) ((x) * 1000)
#define _mv(x) (_uv(x) * 1000)
#define _v(x) (_mv(x) * 1000)

/*dmm bank select, for oid, we'll do open test at first,
then do r_auto for detail in case it's not open*/

#define MODE_IS_V(mode) ((mode & 0xf0) == 0x10)
#define MODE_IS_R(mode) ((mode & 0xf0) == 0x20)

enum {
	DMM_V_AUTO = 0x10, /*precision&slow, range: 0~1V2(G=1..512)*/
	DMM_R_AUTO = 0x20, /*precision&slow, range: 0~1K2(I=1mA, G=1..512)*/
	DMM_R_SHORT, /*rough&fast, range: 0~2R (I=1mA, G=512)*/
	DMM_R_OPEN, /*rough&fast, range: 0~120Kohm (I=10uA, G=1)*/
	/*pls add more mode here to keep history compatibility*/
	DMM_OFF = 0x30,
	DMM_KEEP = 0x00,
};

/* calibration procedure:
1, apply a reference resistor/voltage to the input
2, set register mode.bank, mode.cal, value to do calibration until flag mode.ready is set.
incase some error, ecode is set in mode.ecode
*/
struct dmm_reg_s {
	#define DMM_REG_MODE	0
	union {
		struct dmm_mode_s {
			char bank; /*R/W, dmm bank select*/
			char ecode; /*R, dmm system or calibration error*/
			char cal: 1; /*W, to start calibration procedure, auto zero*/
			char ready : 1; /*R, new data ready to use*/
			char clr : 1; /*W, clear ready flag*/
			char over : 1; /*R, over-range*/
			char dac : 1; /*W, adjust dac output*/
		};
		int value;
	} mode;

	#define DMM_REG_DATA	4
	int value; /*R/W*/
};

#define DMM_UV_TO_D(d) ((d) * (int)((4096.0 * 65536) / 1000 / 1200))

#endif
