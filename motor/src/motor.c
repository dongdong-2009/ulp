/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdlib.h>

pid_t *pid_speed;
pid_t *pid_torque;
pid_t *pid_flux;
motor_t *motor;

/*private*/
static motor_status_t motor_status; 

void motor_Init(void)
{
	motor_status = MOTOR_STATUS_STOP;
	pid_speed = pid_Init();
	pid_torque = pid_Init();
	pid_flux = pid_Init();
	smo_Init();
	motor = malloc(sizeof(motor_t));
	
	/*default motor para, change me!!!*/
	motor->rs = 0;
	motor->ld = 0;
	motor->lq = 0;
	motor->pn = 8;
	smo_SetMotor(motor);
	
	/*default RPM*/
	motor_SetRPM(500);
}

void motor_SetSpeed(short speed)
{
	pid_SetRef(pid_speed, speed); 
}

void motor_SetRPM(short rpm)
{
	short speed = rpm*motor->pn/2/60;
	pid_SetRef(pid_speed, speed);
}

void motor_Start(void)
{
	motor_status = MOTOR_STATUS_START;
	vsm_Start();
}

/*this routine will be called periodly by ADC isr*/
void motor_Update(void)
{
}

/*maybe we can use smo to stop the motor more quickly???*/
void motor_Stop(void)
{
	motor_status = MOTOR_STATUS_STOP;
}