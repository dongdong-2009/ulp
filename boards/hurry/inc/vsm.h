/* vsm.h
 * 	dusk@2009 initial version
 */
#ifndef __VSM_H_
#define __VSM_H_

#include "stm32f10x.h"

void vsm_Init(void); //timer1£¨ADC1°¢2 µ»≥ı ºªØ
void vsm_Update(void); 
void vsm_SetVoltage(int Valpha,int Vbeta,int theta);//unit:mV, ref: stm32f10x_svpwm_3shunt.c/SVPWM_3ShuntCalcDutyCycles()
void vsm_GetCurrent(int *Ialpha, int *Ibeta,int theta);//unit:mA

#endif /*__VSM_H_*/