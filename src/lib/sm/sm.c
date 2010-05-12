/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sm/stepmotor.h"
#include "osd/osd.h"
#include "stm32f10x.h"
#include "capture.h"
#include "flash.h"
#include "smctrl.h"
#include "key.h"
#include "time.h"

//private data read from or store to flash
typedef struct{
	short rpm;//unit: rpm
	short autosteps;
	int dir : 1;
	int runmode : 1;
}sm_config_t;

//private varibles
static int sm_status;
static sm_config_t sm_config;
static time_t sm_stoptimer;

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RESET,
	KEY_RIGHT,
	KEY_LEFT,	
	KEY_NONE
};

void sm_Init(void)
{
	//handle osd display
	int hdlg = osd_ConstructDialog(&sm_dlg);
	osd_SetActiveDialog(hdlg);

	//set key map
	key_SetLocalKeymap(keymap);

	//init for stepper clock
	smctrl_Init();
	
	//init for capture clock
	capture_Init();
		
	//read config from flash and config device
	sm_GetRPMFromFlash(&sm_config.rpm);
	sm_GetAutostepFromFlash(&sm_config.autosteps);

	capture_SetAutoRelaod(sm_config.autosteps);
	sm_SetRPM(sm_config.rpm);
	
	//init for variables
	sm_status = SM_IDLE;
	sm_config.runmode = SM_RUNMODE_MANUAL;
}

void sm_Update(void)
{
	int left, reload, mode, dir;

	left = time_left(sm_stoptimer);
	mode = sm_config.runmode;
	reload = (mode == SM_RUNMODE_MANUAL) ? 65535 : sm_config.autosteps;
	dir = sm_config.dir;
	
	switch (sm_status) {
	case SM_IDLE:
		if(left > 0) {
			capture_SetAutoReload(reload);
			smctrl_SetRotationDirection(dir);
			smctrl_Start();
			sm_status = SM_RUNNING;
		}
		break;

	case SM_RUNNING:
		if((mode == SM_RUNMODE_MANUAL) && (left <= 0)) {
			sm_StopMotor();
		}
		break;

	default:
		sm_status = SM_IDLE;
	}
}

int sm_StartMotor(bool clockwise)
{
	sm_config.dir = clockwise;
	sm_stoptimer = time_get(20); //20ms delay
	return 0;
}

void sm_StopMotor(void)
{
	smctrl_Stop();
	sm_status = SM_IDLE;
	sm_stoptimer = 0;
}

int sm_SetRPM(int rpm)
{
	/*rpm min/max limit*/
	rpm = (rpm < 1) ? 1 : rpm;	
	sm_config.rpm = rpm;	
	smctrl_SetRPM(rpm);

	return 0;
}

int sm_GetRPM(void)
{
	return sm_config.rpm;
}

int sm_SetAutoSteps(int steps)
{
	int result = -1;
	
	steps = (steps < 1) ? 1 : steps;
	
	if(sm_status == SM_IDLE) {
		sm_config.autosteps = steps;
		result = 0;
	}
	
	return result;
}

int sm_GetAutoSteps(void)
{
	return sm_config.autosteps;
}

unsigned short sm_GetSteps(void)
{
	return capture_GetCounter();
}

void sm_ResetStep(void)
{
	if(sm_status == SM_IDLE)
		capture_ResetCounter();
}

int sm_GetRunMode(void)
{
	return sm_config.runmode;
}

int sm_SetRunMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		if((newmode >= SM_RUNMODE_MANUAL) && (newmode < SM_RUNMODE_INVALID))
			sm_config.runmode = newmode;
	}
	
	/*success return 0*/
	return (newmode != sm_config.runmode);
}

int sm_GetRPMFromFlash(int *rpm)
{
	flash_Read((void *)rpm, (void const *)SM_USER_FLASH_ADDR, 4);
        
        return 0;
}

int sm_GetAutostepFromFlash(int *autostep)
{
	flash_Read((void *)autostep, (void const *)(SM_USER_FLASH_ADDR + 4), 4);
        
        return 0;
}

int sm_SaveConfigToFlash(void)
{
	flash_Erase((void *)SM_USER_FLASH_ADDR, 1);
	flash_Write((void *)(&sm_config), (void const *)SM_USER_FLASH_ADDR,8);
        
        return 0;
}

void sm_isr(void)
{
	sm_StopMotor();
}

