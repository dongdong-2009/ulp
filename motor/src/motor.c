/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdlib.h>
#include <string.h>

/*private*/
vector_t Iab, I, Idq;
vector_t Vab, V, Vdq; 
static time_t motor_timer;

void motor_Init(void)
{
	smo_Init();
}

void motor_Update(void)
{
	short speed, rpm, torque_ref;
	
	smo_Update();
	
	/*update cycle = 1s?*/
	if(!motor_timer || time_left(motor_timer) < 0) {
		motor_timer = time_get(MOTOR_UPDATE_PERIOD);
		
		speed = smo_GetSpeed();
		speed = (short)_SPEED(speed);
		rpm = (short)SPEED_TO_RPM(speed, motor->pn);
		
		/*stop motor?*/
		/*the real motor is still run here!!!!!*/
		if(!pid_GetRef(pid_speed) && (rpm < motor->start_rpm) && rpm ) {
			smo_Reset();
			vsm_Stop();
			return;
		}
		
		/*speed pid*/
		torque_ref = pid_Calcu(pid_speed, speed);
		pid_SetRef(pid_flux, 0); /*Id = 0 control method*/
		pid_SetRef(pid_torque, torque_ref);
	}
}

void motor_SetSpeed(short speed)
{
	short cur_speed;
	
	speed = (short)NOR_SPEED(speed);
	pid_SetRef(pid_speed, speed); 
	cur_speed = smo_GetSpeed();
	
	/*start motor*/
	if(!cur_speed && speed) {
		smo_Reset();
		vsm_Start();
	}
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
	smo_Calcu(&V, &I);
	/*6, pid*/
	Vdq.d = pid_Calcu(pid_flux, Idq.d);
	Vdq.q = pid_Calcu(pid_torque, Idq.q);
	/*7, ipark circle lim*/
	/*8, ipark*/
	ipark(&Vdq, &V, angle);
	/*9, iclarke*/
	iclarke(&V, &Vab);
	/*A, set voltage*/
	vsm_SetVoltage(Vab.a, Vab.b);
}