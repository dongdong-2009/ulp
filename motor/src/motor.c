/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdio.h>

pid_t *pid_speed;
pid_t *pid_torque;
pid_t *pid_flux;
vector_t Iab, I, Idq;
vector_t V, Vdq;

/*private*/
motor_status_t stm;
static time_t motor_timer;
static time_t stop_timer;

void motor_Init(void)
{
	stm = MOTOR_IDLE;
	pid_speed = pid_Init();
	pid_torque = pid_Init();
	pid_flux = pid_Init();
	pid_SetRef(pid_speed, 0);
	pid_SetRef(pid_torque, 0);
	pid_SetRef(pid_flux, 0);
	smo_Init();
}

void motor_Update(void)
{
	short speed, rpm, torque_ref, speed_pid;

	speed = smo_GetSpeed();
	speed = (short)_SPEED(speed);
	rpm = (short)SPEED_TO_RPM(speed, motor->pn);
	speed_pid = pid_GetRef(pid_speed);

	switch (stm) {
		case MOTOR_IDLE:
			if(speed_pid > 0)
				stm = MOTOR_START_OP;
			return;

		case MOTOR_START_OP:
			break;
			
		case MOTOR_START:
			smo_Update();
			if(speed_pid == 0) {
				stm = MOTOR_STOP_OP;
				return;
			}
			break;

		case MOTOR_RUN:
			smo_Update();
			if((speed_pid == 0) && (rpm < motor->start_rpm)) {
				stm = MOTOR_STOP_OP;
				return;
			}
			break;

		case MOTOR_STOP_OP:
			smo_Reset();
			vsm_Stop(); /*note: It's not a brake!!!*/
			stop_timer = time_get(MOTOR_STOP_PERIOD);
			stm = MOTOR_STOP;
			return;
			
		case MOTOR_STOP:
			if(time_left(stop_timer) < 0)	
				stm = MOTOR_IDLE;
			return;

		default:
			printf("SYSTEM ERROR!!!\n");
			return;
	}
	
	/*update cycle = 1s?*/
	if(time_left(motor_timer) < 0) {
		motor_timer = time_get(MOTOR_UPDATE_PERIOD);
		
		/*speed pid*/
		torque_ref = pid_Calcu(pid_speed, speed);
		pid_SetRef(pid_flux, 0); /*Id = 0 control method*/
		pid_SetRef(pid_torque, torque_ref);

		if(stm == MOTOR_START_OP) {
			smo_Reset();
			vsm_Start();
			stm = MOTOR_START;
		}
	}
}

void motor_SetSpeed(short speed)
{	
	speed = (short)NOR_SPEED(speed);
	pid_SetRef(pid_speed, speed); 
}

void motor_SetRPM(short rpm)
{
	short speed = (short)RPM_TO_SPEED(rpm, motor->pn);
	pid_SetRef(pid_speed, speed);
}

/*this routine will be called periodly by ADC isr*/
void motor_isr(void)
{
	short angle;
	
	/*1, smo get cur speed&angle*/
	angle = smo_GetAngle();
	/*2, get current*/
	vsm_GetCurrent(&Iab.a, &Iab.b);
	/*3, clarke*/
	clarke(&Iab, &I);
	/*4, park*/
	park(&I, &Idq, angle);
	/*5, update smo*/
	smo_isr(&V, &I);
	/*6, pid*/
	Vdq.d = pid_Calcu(pid_flux, Idq.d);
	Vdq.q = pid_Calcu(pid_torque, Idq.q);
	/*7, ipark circle lim*/
	/*8, ipark*/
	ipark(&Vdq, &V, angle);
	/*9, set voltage*/
	vsm_SetVoltage(V.alpha, V.beta);
}

