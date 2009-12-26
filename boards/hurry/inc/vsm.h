/* vsm.h
 * 	dusk@2009 initial version
 */
#ifndef __VSM_H_
#define __VSM_H_

#include "stm32f10x.h"

#define SECTOR_1	(u32)1
#define SECTOR_2	(u32)2
#define SECTOR_3	(u32)3
#define SECTOR_4	(u32)4
#define SECTOR_5	(u32)5
#define SECTOR_6	(u32)6

void vsm_Init(void); //timer1£¨ADC1°¢2 µ»≥ı ºªØ
void vsm_Update(void); 
void vsm_SetVoltage(int Valpha,int Vbeta);
void vsm_GetCurrent(short *Ialpha, short *Ibeta);
void vsm_Start(void);
void vsm_Stop(void);

#endif /*__VSM_H_*/