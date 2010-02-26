/*
 * 	miaofng@2009 initial version
 */
#ifndef __PID_H_
#define __PID_H_

#include "config.h"

typedef struct {
	short kp;
	short ki;
	short kd; /*not used yet*/
	short ref;
	
	/*private, pls don't touch me;)*/
	int err_history;
} pid_t;

#define PID_GAIN_SHIFT	7 /*n -> range: (+/-) 2^(-n) ~ 2^(15-n)*/
#define NOR_PID_GAIN_VAL	(1.0/(1 << PID_GAIN_SHIFT))
#define NOR_PID_GAIN(g) (g  / NOR_PID_GAIN_VAL)
#define _PID_GAIN(g) (g * NOR_PID_GAIN_VAL)

#define PID_KP_DEF	1.0
#define PID_KI_DEF	0
#define PID_KD_DEF	0

pid_t *pid_Init(void);
void pid_Config(pid_t *pid, short kp, short ki, short kd);
void pid_SetRef(pid_t *pid, short ref);
#define pid_GetRef(pid) (pid->ref)
short pid_Calcu(pid_t *pid, short in);
void pid_Close(pid_t *pid);
#endif /*__PID_H_*/

