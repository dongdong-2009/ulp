/*
 * 	miaofng@2010 initial version
 */
#ifndef __VVT_H_
#define __VVT_H_

#include "pss.h"

/*shared with command shell*/
extern short vvt_gear_advance; //0~10
extern short vvt_knock_pos;
extern short vvt_knock_width;
extern short vvt_knock_strength; //unit: mV
extern short vvt_knock_pattern; //...D C B A

#define vvt_Start() do{ pss_Enable(1); } while(0)
#define vvt_Stop() do{pss_Enable(0);} while(0)

void vvt_Init(void);
void vvt_Update(void);
void vvt_isr(void);
#endif /*__VVT_H_*/

