/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __STEPMOTOR_H_
#define __STEPMOTOR_H_

#include "osd/osd.h"
#include "stm32f10x.h"

/*sm motor status*/
enum {
	SM_IDLE,
	SM_RUNNING,
};

/*sm run modes*/
enum {
	SM_RUNMODE_MANUAL = 0,
	SM_RUNMODE_AUTO,
	SM_RUNMODE_INVALID,
};

extern osd_dialog_t sm_dlg;

void sm_Init(void);
void sm_Update(void);
int sm_StartMotor(bool clockwise);
void sm_StopMotor(void);
int sm_SetSpeed(int rpm);
int sm_GetSpeed(void);
unsigned short sm_GetSteps(void);
void sm_ResetStep(void);
int sm_GetAutoSteps(void);
int sm_SetAutoSteps(int steps);
int sm_GetRunMode(void);
int sm_SetRunMode(int newmode);
void sm_isr(void);
#endif /*__STEPMOTOR_H_*/
