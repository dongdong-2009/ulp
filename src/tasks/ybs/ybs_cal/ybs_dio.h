/*
 * 	miaofng@2013-3-27 initial version
 */

#ifndef __YBS_DIO_H_
#define __YBS_DIO_H_

#include "../ybs.h"

/*float Devation/Gain <=> ybs para conversion*/
#define G2Y(x) ((unsigned short)(x * 0x8000))
#define D2Y(x) ((short)(x * 100.0))
#define Y2G(x) ((float) x / 0x8000)
#define Y2D(x) ((short) x / 100.0)

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

#define MV2GF(mv) (((mv) - 2000) / 160.0)
#define GF2MV(gf) (2000 + (int)((gf) * 160))

struct ybs_info_s {
	char sw[32]; //compile date, NULL terminated
	char sn[12]; //format: 20130328001, NULL terminated
	float Gi; /*calibration: input adc gain coefficient*/
	float Di; /*calibration: input adc zero deviation*/
	float Go; /*calibration: output dac gain coefficient*/
	float Do; /*calibration: output dac zero deviation*/
	struct ybs_mfg_data_s *mfg_data;
};

int ybs_init(struct ybs_info_s *);
int ybs_reset(void);
int ybs_read(float *gf);
int ybs_cal_read(float *gf);
int ybs_cal_write(float gf);
int ybs_cal_config(float Gi, float Di, float Go, float Do);
int ybs_save(void);

#endif
