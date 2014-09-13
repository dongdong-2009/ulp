/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __BSP_H__
#define __BSP_H__

#include "config.h"

void board_init(void);
void board_reset(void);
void trig_set(int high);
int trig_get(void); //vmcomp pulsed
void le_set(int high);
int le_get(void);
void rly_set(int mode);

#endif
