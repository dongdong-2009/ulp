/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "smo.h"

typedef enum {
	RAMP_IN_UPDATE,
	RAMP_IN_ISR
} ramp_type_t;

static time_t smo_timer;
static short smo_speed; /*current motor speed*/
static short smo_angle; /*current motor angle*/
static short smo_locked; /*smo algo is in lock*/

/*used by rampup only*/
static short ramp_speed_inc;
ramp_type_t ramp_type;

static void ramp(ramp_type_t rt);

void smo_Init(void)
{
}

void smo_Update(void)
{
	/*update cycle = 1s?*/
	if(!smo_timer || time_left(smo_timer) < 0) {
		smo_timer = time_get(SMO_UPDATE_PERIOD);
		ramp(RAMP_IN_UPDATE);
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
	ramp_type = RAMP_IN_ISR;
	
	tmp = motor->start_time;
	steps =  (short)(tmp * (VSM_FREQ / 1000));
	ramp_speed_inc = (motor->start_speed) / steps;
	
	if(ramp_speed_inc == 0) {
		/*start progress is too slow, ramp up in update*/
		ramp_type = RAMP_IN_UPDATE;
		
		tmp = motor->start_time;
		steps =  (short)(tmp / SMO_UPDATE_PERIOD);
		ramp_speed_inc = (motor->start_speed) / steps;
	}
	
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
	ramp(RAMP_IN_ISR);
}

/*
*	This routine is used to rampup the motor when startup. because the speed
*	of the motor is so slow that smo algo cann't use the back-EMF to do estimation.
*/
static void ramp(ramp_type_t rt)
{
	int tmp;

	if(smo_locked)
		return;

	/*rampup speed*/
	if(rt == ramp_type) {
		if (smo_speed < motor->start_speed)
			smo_speed += ramp_speed_inc;
	}

	/*rampup angle*/
	if(rt == RAMP_IN_ISR) {
		tmp = smo_speed;
		tmp = tmp << 16;
		tmp = tmp / VSM_FREQ;
		smo_angle += (short)tmp;
	}
}

