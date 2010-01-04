/* vsm.h
 * 	dusk@2009 initial version
 */
#ifndef __VSM_H_
#define __VSM_H_

#include "stm32f10x.h"
#include "pwm.h"

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

#define midShort (1<<15)
#define divSQRT_3 ((short)(0.57735026918963*midShort)) /* 1/sqrt(3) */
#define sqrt3DIV_2 ((short)(0.86602540378444*midShort))

#define PWM2_MODE 0
#define PWM1_MODE 1

/****   Uncomment the Max modulation index     ****/ 
/**** corresponding to the selected PWM frequency ****/
//#define MAX_MODULATION_100_PER_CENT     // up to 11.4 kHz PWM frequency 
//#define MAX_MODULATION_99_PER_CENT      // up to 11.8 kHz
//#define MAX_MODULATION_98_PER_CENT      // up to 12.2 kHz  
//#define MAX_MODULATION_97_PER_CENT      // up to 12.9 kHz  
#define MAX_MODULATION_96_PER_CENT      // up to 14.4 kHz  
//#define MAX_MODULATION_95_PER_CENT      // up to 14.8 kHz
//#define MAX_MODULATION_94_PER_CENT      // up to 15.2 kHz  
//#define MAX_MODULATION_93_PER_CENT      // up to 16.7 kHz
//#define MAX_MODULATION_92_PER_CENT      // up to 17.1 kHz
//#define MAX_MODULATION_91_PER_CENT      // up to 17.5 kHz

#define SAMPLING_TIME_NS (u16)(700) //0.7usec
#define TNOISE_NS (u16)(2550) //2.55usec 
#define TRISE_NS (u16)(2550) //2.55usec

#define SAMPLING_TIME (u16)((SAMPLING_TIME_NS * 72uL)/1000uL)
#define TNOISE (u16)((TNOISE_NS * 72uL)/1000uL)  
#define TRISE (u16)((TRISE_NS * 72uL)/1000uL)
#define TDEAD (u16)((DEADTIME_NS * 72uL)/1000uL)

#define TW_BEFORE SAMPLING_TIME
#define TW_AFTER (u16)(TDEAD+TNOISE)



void vsm_Init(void); //timer1£¬ADC1¡¢2 µÈ³õÊ¼»¯
void vsm_Update(void); 
void vsm_SetVoltage(int alpha,int beta);
void vsm_GetCurrent(int *Ia,int *Ib);
void vsm_Start(void);
void vsm_Stop(void);

#endif /*__VSM_H_*/