/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "stepmotor.h"
#include "osd/osd.h"
#include "stm32f10x.h"
#include "capture.h"
#include "flash.h"
#include "smctrl.h"
#include "key.h"
#include "ulp_time.h"
#include "stdio.h"
#include "led.h"
#include "sys/task.h"

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
	//read config from flash and config device
	sm_GetConfigFromFlash();
	
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
	capture_SetCounter(0);

	//green led flash
	led_flash(LED_GREEN);
	
	//init for variables
	sm_status = SM_IDLE;
	sm_stepcounter = 0;
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
			smctrl_SetRPM(sm_config.rpm);
			smctrl_Start();
			sm_status = SM_RUNNING;
		}
		led_off(LED_RED);
		break;

	case SM_RUNNING:
		if((mode == SM_RUNMODE_MANUAL) && (left <= 0)) {
			sm_StopMotor();
		}
		led_on(LED_RED);
		break;

	default:
		sm_status = SM_IDLE;
	}
}

DECLARE_TASK(sm_Init, sm_Update)

int sm_StartMotor(int clockwise)
{
	if(sm_status == SM_IDLE)
		sm_config.dir = clockwise;
	sm_stoptimer = time_get(13);
	return 0;
}

void sm_StopMotor(void)
{
	int step_cap;
	
	smctrl_Stop();
	sm_status = SM_IDLE;
	sm_stoptimer = 0;
	step_cap = capture_GetCounter();
	capture_SetCounter(0);
	if(sm_config.runmode != SM_RUNMODE_MANUAL)
		step_cap += sm_config.autosteps;
	step_cap = (sm_config.dir == Clockwise) ? step_cap : -step_cap;
	sm_stepcounter += step_cap;
}

int sm_SetRPM(int rpm)
{
	/*rpm min/max limit*/
	rpm = (rpm < 1) ? 1 : rpm;
	rpm = (rpm > SM_MAX_RPM) ? SM_MAX_RPM : rpm;

	sm_config.rpm = rpm;
	if (sm_status == SM_RUNNING) { /*dynamic change the speed of the motor*/
		smctrl_SetRPM(rpm);
	}

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
	int step_cap;
	
	step_cap = capture_GetCounter();
	step_cap = (sm_config.dir == Clockwise) ? step_cap : -step_cap;
	return sm_stepcounter + step_cap;
}

void sm_ResetStep(void)
{
	if (sm_status == SM_IDLE) {
		sm_stepcounter = 0;
	}
}

int sm_GetRunMode(void)
{
	int mode = (sm_config.runmode) ? SM_RUNMODE_AUTO : SM_RUNMODE_MANUAL;
	return mode;
}

int sm_SetRunMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		sm_config.runmode = newmode;
	}
	
	return 0;
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

/*******************************************************************************
* Function Name  : TIM1_UP_IRQHandler
* Description    : This function handles TIM1 overflow and update interrupt 
*                  request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_UP_IRQHandler(void)
{
#if CONFIG_TASK_STEPMOTOR == 1
	/* Clear TIM1 Update interrupt pending bit */
	//TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	TIM1->SR = (uint16_t)~TIM_IT_Update;
	sm_isr();
#endif
}

void sm_isr(void)
{
	/*warnning: manual mode may also reach this point, in case of steps >= 65535*/
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
