/* vsm.h
 * 	dusk@2009 initial version
 */
#ifndef __VSM_H_
#define __VSM_H_

#include "config.h"

#ifndef CONFIG_PWM_FREQ
	#define CONFIG_PWM_FREQ (72000000/5000) /*unit: Hz*/
#endif

enum {
	SECTOR_1 = 1,
	SECTOR_2,
	SECTOR_3,
	SECTOR_4,
	SECTOR_5,
	SECTOR_6
} sector_t;

void vsm_Init(void);
void vsm_Update(void); 
void vsm_SetVoltage(short alpha, short beta); /*normalized alpha/beta voltage*/
void vsm_GetCurrent(short *a, short *b); /*normalized a-b-c current*/
void vsm_Start(void);
void vsm_Stop(void);

#endif /*__VSM_H_*/

