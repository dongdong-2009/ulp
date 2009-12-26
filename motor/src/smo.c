/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdlib.h>

static motor_t *smo_motor;
static short smo_speed; /*current motor speed*/
static short smo_angle; /*current motor angle*/
static short smo_locked; /*smo algo is in lock*/

/*used by rampup only*/
static unsigned smo_start_counter;
static short smo_speed_inc;
static int smo_start_steps;
static int smo_start_speed;

void smo_Init(void)
{
}

void smo_Reset(void)
{
	motor_config_t *config = smo_motor->config;
	
	smo_speed = 0;
	smo_angle = 0;
	smo_locked = 0;
	smo_start_counter = 0;
	
	/*calc speed inc*/
	smo_start_steps = config->duration * config->fs / 1000; 
	smo_start_speed = config->rpm * (60 * 2) / smo_motor->pn;
	smo_speed_inc = (short)(smo_start_speed / smo_start_steps);
}

void smo_SetMotor(motor_t *motor)
{
	smo_motor = motor;
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
	short rpm = smo_speed / (smo_motor->pn/2) *60;
	return rpm;
}

short smo_GetAngle(void)
{
	return smo_angle;
}

/*open loop start motor, standard ramp up curve*/
static short smo_ramp(void)
{
	int tmp;
	motor_config_t *config = smo_motor->config;
	
	smo_start_counter ++;
	if(smo_start_counter < smo_start_steps) {
		smo_speed += smo_speed_inc;
	}
	
	/*det_phi= 2*pi*f*det_t = 2*pi*f*(1/fs) = 2*pi*f/fs*/
	tmp = (int)smo_speed * midShort / config->fs;
	return (short)tmp;
}

void smo_Update(vector_t *pvs, vector_t *pis)
{
	if(!smo_locked) {
		smo_angle += smo_ramp();
	}
	
	/*normal stm update*/
}

