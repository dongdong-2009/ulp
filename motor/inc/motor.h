/*
 * 	miaofng@2009 initial version
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "pid.h"
#include "smo.h"

typedef enum {
	MOTOR_STATUS_ERROR = -1,
	MOTOR_STATUS_STOP = 0,
	MOTOR_STATUS_START,
	MOTOR_STATUS_RUN
} motor_status_t;

/*only for command console to change the settings of these object*/
extern pid_t *pid_speed;
extern pid_t *pid_torque;
extern pid_t *pid_flux;
extern motor_t *motor;

void motor_Init(void);
void motor_SetSpeed(short speed);
void motor_SetRPM(short rpm);
void motor_Start(void);
void motor_Update(void); /*this routine will be called periodly by ADC isr*/
void motor_Stop(void);
#endif /*__MOTOR_H_*/
