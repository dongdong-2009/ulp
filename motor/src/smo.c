/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "smo.h"

static short smo_speed; /*current motor speed*/
static short smo_angle; /*current motor angle*/
static short smo_locked; /*smo algo is in lock*/

/*used by rampup only*/
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
	
	/*var init*/
	smo_speed = 0;
	smo_angle = 0;
	smo_locked = 0;
	
	tmp = motor->start_time;
	steps =  (short)(tmp * (VSM_FREQ / 1000));
	ramp_speed_inc = (motor->start_speed) / steps;
	
	if(ramp_speed_inc == 0)
		ramp_speed_inc = 1;
	
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
	return smo_speed;
}

short smo_GetAngle(void)
{
	return smo_angle;
}

void smo_isr(vector_t *pvs, vector_t *pis)
{
	if(!smo_locked) {
		int tmp;
		/*
		*	This routine is used to rampup the motor when startup. because the speed
		*	of the motor is so slow that smo algo cann't use the back-EMF to do estimation.
		*/
		if (smo_speed < motor->start_speed)
			smo_speed += ramp_speed_inc;
		
		tmp = smo_speed;
		tmp = tmp << 16;
		tmp = tmp / VSM_FREQ;
		tmp = _SPEED(tmp);
		smo_angle += (short)tmp;
	}
}

