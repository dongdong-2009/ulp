/*
 * 	miaofng@2009 initial version
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "motor/pid.h"
#include "motor/smo.h"
#include "motor/vector.h"
#include "motor/math.h"

/*shared with command shell*/
extern vector_t Iab, I, Idq;
extern vector_t V, Vdq; 
extern pid_t *pid_speed;
extern pid_t *pid_torque;
extern pid_t *pid_flux;
extern short Vramp;
#define MOTOR_STOP_PERIOD	(500) /*unit: mS*/

#define RPM_TO_SPEED(rpm) ((int)rpm * motor->pn / (2 * 60))
#define SPEED_TO_RPM(speed) ((int)speed * (60 * 2) / motor->pn)

typedef enum {
	MOTOR_IDLE, /*green*/
	MOTOR_START_OP,
	MOTOR_START, /*yellow flash*/
	MOTOR_RUN, /*green flash*/
	MOTOR_STOP_OP,
	MOTOR_STOP, /*yellow*/
	MOTOR_ERROR /*red flash*/
} motor_status_t;

void motor_Init(void);
void motor_SetSpeed(short speed);
void motor_Update(void);
void motor_Start(void);
motor_status_t motor_GetStatus(void);
void motor_isr(void); /*this routine will be called periodly by ADC isr*/
#endif /*__MOTOR_H_*/

