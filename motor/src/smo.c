/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "smo.h"

static short smo_speed;
static short smo_angle;
static short smo_locked; /*smo algo is in lock*/

static vector_t smo_bemf;
static short smo_A; /* (-Rs * Ts) / Ls */
static short smo_B; /* (Ts / Ls) * (NOR_VOL_VAL / NOR_AMP_VAL) */

/*used by rampup only*/
static short ramp_speed;
static short ramp_angle;
static short ramp_speed_inc;

void smo_Init(void)
{
}

void smo_Update(void)
{
}

void smo_Start(void)
{	
	int tmp, steps;
	float r, l, g;
	
	/*ramp var init*/
	ramp_speed = 0;
	ramp_angle = 0;
	smo_locked = 0;
	
	tmp = motor->start_time;
	steps =  (short)(tmp * (VSM_FREQ / 1000));
	ramp_speed_inc = (motor->start_speed) / steps;
	
	if(ramp_speed_inc == 0)
		ramp_speed_inc = 1;

	/*smo var init*/
	smo_speed = 0;
	smo_angle = 0;
	smo_bemf.alpha = 0;
	smo_bemf.beta = 0;

	r = _RES(motor->rs);
	l = _IND(motor->ld) / 1000;
	g = -1.0 * r / l / VSM_FREQ;
	smo_A = (int) NOR_SMO_GAIN(g);
	g = 1.0 * NOR_VOL_VAL / NOR_AMP_VAL / l / VSM_FREQ;
	smo_B = (int) NOR_SMO_GAIN(g);
	
	/*pid gain para init according to the given motor model*/
}

/*return yes(1)/no(0)*/
int smo_IsLocked(void)
{
	return smo_locked;
}

 /*unit: Hz*/
short smo_GetSpeed(void)
{
	short speed;
	speed = (smo_locked)?smo_speed : smo_angle;
	return speed;
}

short smo_GetAngle(void)
{
	short angle;
	angle = (smo_locked)?smo_angle : ramp_angle;
	return angle;
}

void smo_isr(vector_t *pvs, vector_t *pis)
{
	vector_t ise;
	
	if(!smo_locked) {
		int tmp;
		/*
		*	This routine is used to rampup the motor when startup. because the speed
		*	of the motor is so slow that smo algo cann't use the back-EMF to do estimation.
		*/
		if (ramp_speed < motor->start_speed)
			ramp_speed += ramp_speed_inc;
		
		tmp = ramp_speed;
		tmp = tmp << 16;
		tmp = tmp / VSM_FREQ;
		tmp = _SPEED(tmp);
		ramp_angle += (short)tmp;
	}

	/*1, predict current Ise*/
	/*2, compare the Ise with current Is to obtain bemf*/
	/*3, calculate smo_angle*/
	/*4, calculate smo_speed*/
}

