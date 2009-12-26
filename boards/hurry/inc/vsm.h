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

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)2147483648uL)

#define SQRT_3		1.732051
#define T		(PWM_PERIOD * 4)
#define T_SQRT3         (u16)(T * SQRT_3)

#define PWM2_MODE 0
#define PWM1_MODE 1

void vsm_Init(void); //timer1£¨ADC1°¢2 µ»≥ı ºªØ
void vsm_Update(void); 
void vsm_SetVoltage(int Valpha,int Vbeta);
void vsm_GetCurrent(short *Ialpha, short *Ibeta);
void vsm_Start(void);
void vsm_Stop(void);

#endif /*__VSM_H_*/