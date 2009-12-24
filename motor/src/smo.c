/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "board.h"
#include "motor.h"
#include <stdlib.h>

void smo_Init(void)
{
}

void smo_SetMotor(motor_t *motor)
{
}

void smo_Update(vector_t *pvs, vector_t *pis)
{
}

/*return yes(1)/no(0)*/
int smo_IsReady(void)
{
	return 0;
}

 /*unit: Hz*/
short smo_GetSpeed(void)
{
	return 0;
}

/*unit: RPM*/
short smo_GetRPM(void)
{
	return 0;
}

short smo_GetAngle(void)
{
	return 0;
}