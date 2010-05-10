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

//private data read from or store to flash
typedef struct{
	int sm_rpm;//unit: rpm
	int sm_autosteps;
}sm_config_t;

//private varibles
static int sm_status;
static int sm_runmode;
static sm_config_t sm_config;
static bool sm_startevent;
static bool sm_stopevent;

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RESET,
	KEY_RIGHT,
	KEY_LEFT,	
	KEY_NONE
};

static bool sm_Process(void);

static bool sm_Process(void)
{
	bool result;
	
	switch(sm_status){
	case SM_IDLE:
		if(sm_startevent){
			sm_status = SM_RUNNING;
			capture_SetAutoRelaod(sm_config.sm_autosteps);
			sm_startevent = FALSE;
			result = TRUE;
		}
		else
			result = FALSE;
		break;
	case SM_RUNNING:
		if(sm_stopevent){
			sm_status = SM_IDLE;
			sm_stopevent = FALSE;
			result = TRUE;
		}
		else
			result = FALSE;
		break;
	default:
		break;
	}

	return result;
}

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
	sm_GetRPMFromFlash(&sm_config.sm_rpm);
	sm_GetAutostepFromFlash(&sm_config.sm_autosteps);

	capture_SetAutoRelaod(sm_config.sm_autosteps);
	sm_SetRPM(sm_config.sm_rpm);
	
	//init for variables
	sm_status = SM_IDLE;
	sm_runmode = SM_RUNMODE_MANUAL;
	sm_startevent = FALSE;
	sm_stopevent = FALSE;
}

void sm_Update(void)
{

}

int sm_StartMotor(bool clockwise)
{
	// there should be a state machine operation here ....
	sm_startevent = TRUE;
	if(sm_Process()){
		smctrl_SetRotationDirection(clockwise);
		smctrl_Start();
	}
	return 0;
}

void sm_StopMotor(void)
{
	// there should be a state machine operation here ....
	sm_stopevent = TRUE;
	if(sm_Process()){
		smctrl_Stop();
	}
}

int sm_SetRPM(int rpm)
{
	/*rpm min/max limit*/
	rpm = (rpm < 1) ? 1 : rpm;	
	sm_config.sm_rpm = rpm;	
	smctrl_SetRPM(rpm);

	return 0;
}

int sm_GetRPM(void)
{
	return sm_config.sm_rpm;
}

int sm_SetAutoSteps(int steps)
{
	int result = -1;
	
	steps = (steps < 1) ? 1 : steps;
	
	if(sm_status == SM_IDLE) {
		sm_config.sm_autosteps = steps;
		result = 0;
	}
	
	return result;
}

int sm_GetAutoSteps(void)
{
	return sm_config.sm_autosteps;
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
	return sm_runmode;
}

int sm_SetRunMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		if((newmode >= SM_RUNMODE_MANUAL) && (newmode < SM_RUNMODE_INVALID))
			sm_runmode = newmode;
	}
	
	/*success return 0*/
	return (newmode != sm_runmode);
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
	sm_stopevent = TRUE;
	if(sm_Process()){
		smctrl_Stop();
	}
}

