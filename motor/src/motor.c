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
static vector_t Iab, I , Idq;
static vector_t Vdq, V;
static time_t motor_timer;

void motor_Init(void)
{
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

void motor_Update(void)
{
	short speed, rpm, torque_ref;
	
	/*update cycle = 1s*/
	if(!motor_timer || time_left(motor_timer) < 0) {
		motor_timer = time_get(1000);
		
		speed = smo_GetSpeed();
		rpm = smo_GetRPM();
		
		/*stop motor?*/
		if(!pid_GetRef(pid_speed) && (rpm < 100) && (rpm > 0))
			vsm_Stop();
		
		/*speed pid && set torque/flux pid ref according to the speed*/
		torque_ref = pid_Calcu(pid_speed, speed);
		pid_SetRef(pid_flux, 0); /*Id = 0 control method*/
		pid_SetRef(pid_torque, torque_ref);
	}
}

void motor_SetSpeed(short speed)
{
	short cur_speed;
	pid_SetRef(pid_speed, speed); 
	cur_speed = smo_GetSpeed();
	if(!cur_speed && speed) vsm_Start();
}

void motor_SetRPM(short rpm)
{
	short speed = rpm*motor->pn/2/60;
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
	smo_Update(&V, &I);
	/*6, pid*/
	Vdq.d = pid_Calcu(pid_flux, Idq.d);
	Vdq.q = pid_Calcu(pid_torque, Idq.q);
	/*7, ipark circle lim*/
	/*8, ipark*/
	ipark(&Vdq, &V, angle);
	/*9, set voltage*/
	vsm_SetVoltage(V.alpha, V.beta);
}