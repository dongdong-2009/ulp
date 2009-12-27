/*
 * 	miaofng@2009 initial version
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "pid.h"
#include "smo.h"
#include "normalize.h"

/*readonly, for debug purpose*/
extern vector_t Iab, I, Idq;
extern vector_t Vab, V, Vdq; 

#define MOTOR_UPDATE_PERIOD	(1000) /*unit: mS*/

void motor_Init(void);
void motor_SetSpeed(short speed);
void motor_SetRPM(short rpm);
void motor_Update(void);
void motor_isr(void); /*this routine will be called periodly by ADC isr*/
#endif /*__MOTOR_H_*/
