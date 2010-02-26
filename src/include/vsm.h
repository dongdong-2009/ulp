/* vsm.h
*	dusk@2009 initial version
*/
#ifndef __VSM_H_
#define __VSM_H_

#include "config.h"

#define T 2500 /*72,000,000 / 2500 / 2 = 14, 400 Hz*/
#define TD 800 /*time of dead zone, unit: nS, range: 0~1763*/

/*current sense resistor and adc ref voltage*/
#define VSM_VDC_RATIO (1/10.0)
#define VSM_RS_OHM (4.7)
#define VSM_VREF_MV (3300.0)

#define VSM_FREQ (CONFIG_SYS_FREQ/T/2)

typedef enum {
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

