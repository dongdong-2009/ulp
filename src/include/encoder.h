/*
 * 	miaofng@2011 initial version
 */
#ifndef __ENCODER_H_
#define __ENCODER_H_

#include "config.h"

int encoder_Init(void);
int encoder_SetRange(int min, int max);
int encoder_GetValue(void);
int encoder_SetValue(int);
int encoder_GetSpeed(void);

//hardware driver
int encoder_hwInit(void);
int encoder_hwGetValue(void);

#endif /*__ENCODER_H_*/
