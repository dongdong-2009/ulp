/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __DPS_H__
#define __DPS_H__

#include "config.h"

void dps_auto_regulate(void);
void dps_can_handler(can_msg_t *msg);

/*dps bsp routines*/
void dps_drv_init(void);

void lv_enable(int yes);
void lv_u_set(float v);

void is_i_set(float A);
float is_i_get(void);

#endif
