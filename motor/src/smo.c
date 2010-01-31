/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdlib.h>
#include <string.h>

motor_t *motor;

static time_t smo_timer;

static short smo_speed; /*current motor speed*/
static short smo_angle; /*current motor angle*/
static short smo_locked; /*smo algo is in lock*/

/*used by rampup only*/
static short smo_start_speed; /*final rampup speed*/
static short smo_start_speed_inc; /*rampup speed inc per step*/
static short smo_start_flag; /*1 ramp in update, 0 ramp in isr*/

void smo_Init(void)
{
	motor = malloc(sizeof(motor_t));
	
	/*default motor para, change me!!!*/
	motor->rs = 0;
	motor->ld = 0;
	motor->lq = 0;
	motor->pn = 8;
	motor->start_time = CONFIG_MOTOR_START_TIME;
	motor->start_rpm = CONFIG_MOTOR_START_RPM;
}

void smo_Update(void)
{
	/*update cycle = 1s?*/
	if(!smo_timer || time_left(smo_timer) < 0) {
		smo_timer = time_get(SMO_UPDATE_PERIOD);
		
		/*update smo speed when in ramp-up progress */
		if(!smo_locked && smo_start_flag) {
			if (smo_speed < smo_start_speed)
				smo_speed += smo_start_speed_inc;
		}
	}
}

void smo_Reset(void)
{	
	int tmp, steps;
	
	/*var init*/
	smo_speed = 0;
	smo_angle = 0;
	smo_locked = 0;
	
	/*rampup var init*/
	smo_start_flag = 0; /*ramp up in isr*/
	
	tmp = motor->start_rpm;
	tmp = RPM_TO_SPEED(tmp, motor->pn);
	smo_start_speed = (short) NOR_SPEED(tmp);
	
	tmp = motor->start_time;
	steps =  (short)(tmp * CONFIG_PWM_FREQ / 1000);
	smo_start_speed_inc = (smo_start_speed) / steps;
	
	if(smo_start_speed_inc == 0) {
		/*start progress is too slow, ramp up in update*/
		smo_start_flag = 1;
		
		tmp = motor->start_time;
		steps =  (short)(tmp / SMO_UPDATE_PERIOD);
		smo_start_speed_inc = (smo_start_speed) / steps;
	}
	
	/*pid gain para init according to the given motor model*/
}

/*return yes(1)/no(0)*/
int smo_IsReady(void)
{
	return smo_locked;
}

 /*unit: Hz*/
short smo_GetSpeed(void)
{
	return smo_speed;
}

/*unit: RPM*/
short smo_GetRPM(void)
{
	short rpm = (short) SPEED_TO_RPM(smo_speed, motor->pn);
	return rpm;
}

short smo_GetAngle(void)
{
	return smo_angle;
}

/*motor speed ramp up, return angle*/
static short smo_ramp(void)
{
	int tmp;
	
	if(!smo_start_flag) {
		if (smo_speed < smo_start_speed)
			smo_speed += smo_start_speed_inc;
	}
	
	/*det_phi= 2*pi*f*det_t = 2*pi*f*(1/fs) = 2*pi*f/fs*/
	tmp = smo_speed;
	tmp = tmp << 16;
	tmp = tmp / CONFIG_PWM_FREQ;
	return (short)tmp;
}

void smo_isr(vector_t *pvs, vector_t *pis)
{
	if(!smo_locked) {
		smo_angle += smo_ramp();
	}
	
	/*normal stm update*/
}

