/*
 * 	miaofng@2009 initial version
 */
#ifndef __NORMALIZE_H_
#define __NORMALIZE_H_

#include "config.h"
#include "math.h"

/*note: NOR_XX must be the maximum allowable value*/
#define NOR_RES_VAL	(1<<10) /*1024 Ohm*/
#define NOR_RES(x) (midShort/NOR_RES_VAL*x)
#define _RES(y) (((int)y*NOR_RES_VAL) >> 15))

#define NOR_IND_VAL	(1<<10)/*1024 mH*/
#define NOR_IND(x) (midShort/NOR_IND_VAL*x)
#define _IND(y) (((int)y*NOR_IND_VAL) >> 15))

#define NOR_VOL_VAL	(1<<10) /*1024 mV*/
#define NOR_VOL(x) (midShort/NOR_VOL_VAL*x)
#define _VOL(y) (((int)y*NOR_VOL_VAL) >> 15))

#define NOR_AMP_VAL	(1<<13) /*8192 mA*/
#define NOR_AMP(x) (midShort/NOR_AMP_VAL*x)
#define _AMP(y) (((int)y*NOR_AMP_VAL) >> 15))

#endif /*__NORMALIZE_H_*/
