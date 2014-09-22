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
	DPS_HV,
	DPS_IS,
};

typedef struct {
	unsigned char cmd;
	unsigned char dps;
	unsigned char key;
	unsigned char unused1;
	float value;
} dps_cfg_msg_t;

void dps_init(void);
int dps_config(int key, float v);

#endif
