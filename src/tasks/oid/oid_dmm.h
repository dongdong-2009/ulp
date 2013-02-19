/*
 * 	miaofng@2013 initial version
 */

#ifndef __OID_H__
#define __OID_H__

/*dmm bank select, for oid, we'll do open test at first,
then do r_auto for detail in case it's not open*/

#define MODE_IS_V(mode) ((mode & 0xf0) == 0x10)
#define MODE_IS_R(mode) ((mode & 0xf0) == 0x20)

enum {
	DMM_V_AUTO = 0x10, /*precision&slow, range: 1uV ~ 1V*/
	DMM_R_AUTO = 0x20, /*precision&slow, range: 1mOhm ~ 100Kohm*/
	DMM_R_SHORT, /*rough&fast, range: 1~2000mohm (I=1mA, G=512)*/
	DMM_R_OPEN, /*rough&fast, range: 50~120Kohm (I=10uA, G=1)*/
	/*pls add more mode here to keep compatibility*/
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
		};
		int value;
	} mode;

	#define DMM_REG_DATA	1
	int value; /*R/W*/

	#define DMM_REG_ADC	2
	union {
		struct dmm_adc_s {
			int iexc_uA: 16; /*0-1000*/
			int pga : 16; /*0-512*/
		};
		int value;
	} adc;
};

#endif
