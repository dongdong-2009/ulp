/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __STEPMOTOR_H_
#define __STEPMOTOR_H_

#include "osd/osd.h"
#include "stm32f10x.h"
#include "l6208.h"

//data read from or store to flash
typedef struct{
	int rpm;//unit: rpm
	int autosteps;
	int runmode;
	l6208_config_t l6208_cfg;
}sm_config_t;

/*sm run modes*/
enum {
	SM_RUNMODE_MANUAL = 0,
	SM_RUNMODE_AUTO,
	SM_RUNMODE_INVALID,
};
/*define max RPM value*/
#define SM_MAX_RPM 250

extern osd_dialog_t sm_dlg;

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
int sm_SetStepMode(int newmode);
int sm_GetStepMode(void);
int sm_SetDecayMode(int newmode);
int sm_GetDecayMode(void);
#endif /*__STEPMOTOR_H_*/
