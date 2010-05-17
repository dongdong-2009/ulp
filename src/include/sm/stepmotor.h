/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __STEPMOTOR_H_
#define __STEPMOTOR_H_

#include "osd/osd.h"
#include "stm32f10x.h"

//126k,127k
#define SM_USER_FLASH_ADDR 0x0801f800

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
int sm_StartMotor(int clockwise);
void sm_StopMotor(void);
int sm_SetRPM(int rpm);
int sm_GetRPM(void);
int sm_GetSteps(void);
void sm_ResetStep(void);
int sm_GetAutoSteps(void);
int sm_SetAutoSteps(int steps);
int sm_GetRunMode(void);
int sm_SetRunMode(int newmode);
int sm_GetConfigFromFlash(void);
int sm_SaveConfigToFlash(void);
void sm_isr(void);
#endif /*__STEPMOTOR_H_*/
