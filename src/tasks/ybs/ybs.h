/*
*
* miaofng@2013-9-26
*
*/

#ifndef __YBS_H__
#define __YBS_H__

#include "config.h"

/*
 * Fmax = 50gf(standard, convert to yarn force is 100gf)
 * Vmax = 10V(standard) .. 11.43V(real dac can reached)
 * Vmin = 2V(standard)
 * MV_PER_GF = 160mV(standard)
 * DAC = 10bit
 * DAC_PER_BITmin = 11.1655 mV(real)
 * DAC_PER_BITmin = 69.7847 mgf(real)
 */

#define AVOFS  0.00 /*unit: V*/
#define AGAIN  10.0 /*unit: v/v */
#define YVOFS  2.00 /*unit: V*/

#ifdef CONFIG_YBS_FOR_PCTEST
#define YGAIN  0.10 /*unit: V/gf*/
#else
#define YGAIN  0.16 /*unit: V/gf*/
#endif

/*
#define MV2GF(mv) (((mv) - (int)(YVOFS*1000)) / (int)(YGAIN*1000)
#define GF2MV(gf) ((int)(YVOFS*1000) + (int)((gf) * (int)(YGAIN*1000)))
*/

/*
ybs general formulas:
1, gf = gf_rough * Gi + Di
      = [(adc_value + swdi) * swgi] * Gi + Di
      = adc_value * (swgi * Gi) + (swdi * swgi * Gi + Di)
      = adc_value * ybs_gi + ybs_di
2, vout = dac_vout * GA + DA
	= (dac_value / YBSD_DAC_V2D(1.0)) * GA + DA
	= ([(gf * Go + Do) * swgo + swdo] / YBSD_DAC_V2D(1.0)) * GA + DA
3, vout = gf * GY + DY !!!ybs design specification

note:
gf_rough	un-calibrated strain force, unit: gf
gf		strain force after calibration
Gi Di Go Do	calibration parameters Gain&Deviation
swdi		ybs input deviation, got when reset key is pressed
swgi		ybs input gain, gf_rough = (adc_value + swdi) * swgi
hwgi		spring head gain, or named as strain bridge sensitivity, unit: mv/gf
swgo swdo	ybs output gain&deviation, dac_value = (gf * Go + Do) * swgo + swdo
vout		ybs output voltage, unit: V
dac_vout	mcu dac output voltage, unit: V
GA DA		external analog amplifier circuit gain & offset, vout = dac_vout * GA + DA
		in ideal situation DA = 0V
GY DY		ybs design specification
*/
struct ybs_mfg_data_s {
	char cksum;
	char sn[15]; //format: 20130328001, NULL terminated
	unsigned flag;

	/*ybs design specification*/
	float GA; //amplifier circuit gain, 10
	float DA; //amplifier circuit offset, 0v
	float GY; //ybs gain, 0.16v/gf
	float DY; //ybs offset, 2.00v

	float hwgi; //spring head gain, unit: v/gf
	float swdi; //autoset by reset key, unit: 24bit digital

	/*calibration parameters*/
	unsigned date; //seconds since Jan. 1, 1970, midnight GMT
	float Gi; //0.0001~1.9999
	float Di; //unit: gf
	float Go; //0.0001~1.9999
	float Do; //unit: gf
};

enum {
	YBS_UNLOCK_NONE,
	//please add new ybs unlock type here ...

	YBS_UNLOCK_ALL,
};

#endif
