/*
 * 	miaofng@2009 initial version
 */
#ifndef __NORMALIZE_H_
#define __NORMALIZE_H_

#include "config.h"

#ifndef CONFIG_SYS_FREQ
	#define CONFIG_SYS_FREQ (72000000) /*unit: Hz*/
#endif

/*constant*/
#define maxShort (1<<16)
#define midShort (1<<15)
#define divSQRT_3 ((short)(0.57735026918963*midShort)) /* 1/sqrt(3) */
#define sqrt3DIV_2 ((short)(0.86602540378444*midShort))

/*note: NOR_XX must be the maximum allowable value*/
#define NOR_RES_VAL	(1<<10) /*unit: Ohm*/
#define NOR_RES(x) ((int)x << (15 -10)) /*y =  (x/nor_res)*32768*/
#define _RES(y) ((int)y >> (15 - 10)) /* res= (y/32768)*nor_res */

#define NOR_IND_VAL	(1 << 10)/*unit: mH*/
#define NOR_IND(x) ((int)x << (15 - 10)) /*y =  (x/nor_ind)*32768*/
#define _IND(y) ((int)y >> (15 - 10)) /* ind= (y/32768)*nor_ind */

#define NOR_VOL_VAL	(1 << 20) /*unit: mV*/
#define NOR_VOL(x) ((int)x >> (20 - 15)) /*y =  (x/nor_vol)*32768*/
#define _VOL(y) ((int)y << (20 - 15)) /* vol = (y/32768)*nor_vol */

#define NOR_AMP_VAL	(1 << 13) /*unit: mA*/
#define NOR_AMP(x) ((int)x << (15 - 13)) /*y =  (x/nor_amp)*32768*/
#define _AMP(y) ((int)y >> (15 - 13)) /* amp = (y/32768)*nor_amp */

#define NOR_SPEED_VAL (1 << 10) /*unit: Hz*/
#define NOR_SPEED(x) ((int)x << (15 - 10)) /*y =  (x/nor_speed)*32768*/
#define _SPEED(y) ((int)y >> (15 - 10)) /* speed = (y/32768)*nor_speed */

#endif /*__NORMALIZE_H_*/
