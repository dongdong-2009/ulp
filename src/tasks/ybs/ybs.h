/*
*
* miaofng@2013-9-26
*
*/

#ifndef __YBS_H__
#define __YBS_H__

/*float Devation/Gain <=> ybs para conversion*/
#define G2Y(fg) ((unsigned short)(fg * 0x8000))
#define D2Y(fd) ((short)(fd * 100.0))
#define Y2G(ig) ((float) ig / 0x8000)
#define Y2D(id) ((short) id / 100.0)

/*
 * Fmax = 50gf(standard, convert to yarn force is 100gf)
 * Vmax = 10V(standard) .. 11.43V(real dac can reached)
 * Vmin = 2V(standard)
 * MV_PER_GF = 160mV(standard)
 * DAC = 10bit
 * DAC_PER_BITmin = 11.1655 mV(real)
 * DAC_PER_BITmin = 69.7847 mgf(real)
 */

//#define MV2MGF(mv)  (((mv) - 2000) * 1000 / 160)
//#define MGF2MV(mgf) (2000 + ((mgf) * 160) / 1000)

#define AVOFS  0.00 /*unit: V*/
#define AGAIN  10.0 /*unit: v/v */
#define YVOFS  2.00 /*unit: V*/
#define YGAIN  0.1 /*unit: V/gf*/

#define MV2GF(mv) (((mv) - 2000) / 160.0)
#define GF2MV(gf) (2000 + (int)((gf) * 160))

enum {
	YBS_E_OK,
	YBS_E_CFG,
};

enum {
	YBS_CMD_RESET,
	YBS_CMD_SAVE,
	YBS_CMD_UNLOCK,
};

enum {
	REG_CMD, //like
	REG_CAL = REG_CMD + 2,
	REG_SYS = REG_CAL + 16,
};

/*
note:
1, clip is a sub function of digital ybs sensor
2, analog ybs sensor could be changed to digital ybs sensor by an special key sequece
3, gf_rough = (adc_value + swdi) * swgi, swdi is autoset when reset key is pressed
4, gf_digi = gf_rough * Gi + Di
5, dac_value = (gf_digi * Go + Do) * swgi + swdi
*/
struct ybs_cfg_s {
	char cksum;
	char sn[15]; //format: 130328001, NULL terminated

	/*calibration parameters*/
	float Gi; //0.0001~1.9999
	float Di; //unit: gf
	float Go; //0.0001~1.9999
	float Do; //unit: gf

	/*ybs_a design spec*/
	float GA; //amplifier circuit gain, 10
	float DA; //amplifier circuit offset, 0v
	float GY; //ybs gain, 0.16v/gf
	float DY; //ybs offset, 2.00v

	float hwgi; //spring head gain, unit: v/gf
	float swdi; //autoset by reset key, unit: digital
};


void ybs_isr(void);

#endif
