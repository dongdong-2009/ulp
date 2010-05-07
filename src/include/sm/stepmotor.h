/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __STEPMOTOR_H_
#define __STEPMOTOR_H_

#include "osd/osd.h"

/*sm run modes*/
enum {
	SM_RUNMODE_MANUAL = 0,
	SM_RUNMODE_AUTO,
	SM_RUNMODE_INVALID,
};

typedef enum{
	RPM_INC = 0,
	RPM_DEC,
	RPM_RESET,
	RPM_OK
}sm_rpm_t;

typedef enum{
	STEP_INC = 0,
	STEP_DEC,
	STEP_RESET,
	STEP_OK
}sm_autostep_t;

extern osd_dialog_t sm_dlg;

void sm_Init(void);
void sm_Update(void);
void sm_StartMotor(void);
void sm_StopMotor(void);
void sm_SetSpeed(sm_rpm_t sm_rpm);
int sm_GetSpeed(void);
unsigned short sm_GetSteps(void);
void sm_ResetStep(void);
unsigned short sm_GetAutoSteps(void);
void sm_SetAutoSteps(sm_autostep_t autostep);
int sm_GetRunMode(void);
int sm_SetRunMode(int newmode);
void sm_isr(void);
#endif /*__STEPMOTOR_H_*/
