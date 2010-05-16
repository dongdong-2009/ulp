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
#include "stdio.h"

//private data read from or store to flash
typedef struct{
	short rpm;//unit: rpm
	short autosteps;
	int dir : 1;
	int runmode : 1;
}sm_config_t;

enum{
	Clockwise = 0,
	CounterClockwise
};

//private varibles
static int sm_status;
static sm_config_t sm_config;
static time_t sm_stoptimer;
static int sm_dirswitch;
static int sm_stepcounter;

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

void sm_Init(void)
{
#ifdef CONFIG_TASK_OSD
	//handle osd display
	int hdlg = osd_ConstructDialog(&sm_dlg);
	osd_SetActiveDialog(hdlg);

	//set key map
	key_SetLocalKeymap(keymap);
#endif
	//init for stepper clock
	smctrl_Init();
	
	//init for capture clock
	capture_Init();
	capture_Start();
	
	//read config from flash and config device
	sm_GetConfigFromFlash();
	capture_SetAutoReload(sm_config.autosteps);
	sm_SetRPM(sm_config.rpm);
	smctrl_Stop();
	
	//init for variables
	sm_status = SM_IDLE;
	sm_dirswitch = 0;
	sm_stepcounter = 0;
#if 0
	sm_config.runmode = SM_RUNMODE_MANUAL;
	sm_config.rpm = 60;
	sm_config.autosteps = 2000;
	sm_config.dir = 1;
	capture_SetAutoReload(sm_config.autosteps);
	sm_SetRPM(sm_config.rpm);	
#endif
	
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
		//printf("counter: %d \n",capture_GetCounter());
		break;

	default:
		sm_status = SM_IDLE;
	}
}

int sm_StartMotor(int clockwise)
{
	if (sm_config.runmode == SM_RUNMODE_MANUAL) {
		if (clockwise != sm_config.dir) 
			sm_dirswitch = 1;
		else
			sm_dirswitch = 0;
	}
	
	//sm_config.dir = clockwise;
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

int sm_GetSteps(void)
{
	int temp;

	switch (sm_config.dir) {
	case Clockwise:
		if (sm_dirswitch == 1) {
			sm_stepcounter += capture_GetCounter();
			temp = sm_stepcounter;
			capture_ResetCounter();
			sm_config.dir = CounterClockwise;
			sm_dirswitch = 0;
		}
		else
			temp = sm_stepcounter + capture_GetCounter();
		break;
		
	case CounterClockwise:
		if(sm_dirswitch == 1) {
			sm_stepcounter -= capture_GetCounter();
			temp = sm_stepcounter;
			capture_ResetCounter();
			sm_config.dir = Clockwise;
			sm_dirswitch = 0;
		}
		else
			temp = sm_stepcounter - capture_GetCounter();
		break;
		
	default:
		break;
	}
	
	return temp;
}

void sm_ResetStep(void)
{
	if (sm_status == SM_IDLE) {
		sm_stepcounter = 0;
		capture_ResetCounter();
	}
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

int sm_GetConfigFromFlash(void)
{
	flash_Read((void *)(&sm_config), (void const *)SM_USER_FLASH_ADDR, sizeof(sm_config_t));
        
	return 0;
}
int sm_SaveConfigToFlash(void)
{
	flash_Erase((void *)SM_USER_FLASH_ADDR, 1);
	flash_Write((void *)SM_USER_FLASH_ADDR, (void const *)(&sm_config), sizeof(sm_config_t));
	
	return 0;
}

void sm_isr(void)
{
	if (sm_config.runmode == SM_RUNMODE_MANUAL) {
		sm_stepcounter = 0;
	}
	sm_StopMotor();
}


#if 0
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_sm_func(int argc, char *argv[])
{
#if 0
	const char usage[] = { \
		" usage:\n" \
		" sm start,stop\n" \
	};

	if (argc > 0 && argc != 2) {
		printf(usage);
		return 0;
	}

	if (strcmp(argv[1],"start") == 0) {
		sm_StartMotor(1);
	}

	if (strcmp(argv[1],"save") == 0) {
		sm_SaveConfigToFlash();
	}
#endif
	sm_StartMotor(1);
	return 1;
}
const cmd_t cmd_sm = {"sm", cmd_sm_func, "stepper motor debug"};
DECLARE_SHELL_CMD(cmd_sm)
#endif
