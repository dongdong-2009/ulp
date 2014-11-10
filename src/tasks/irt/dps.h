/*
*
*  miaofng@2014-9-17   initial version
*
*/

#ifndef __DPS_H__
#define __DPS_H__

#include "config.h"

enum {
	DPS_LV,
	DPS_HS,
	DPS_IS,
	DPS_VS,
	DPS_HV,
};

enum {
	DPS_V, //U OR I SET
	DPS_P, //PROTECTION OR DC BIAS SET
	DPS_E,
	DPS_G,
};

void dps_init(void);
void dps_update(void);

float dps_sense(int dps);
int dps_gain(int dps, int gain, int execute);
int dps_set(int dps, float v);
int dps_enable(int dps, int enable);
int dps_config(int dps, int key, void *p);

#endif
