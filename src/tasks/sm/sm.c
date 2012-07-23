/*
 * miaofng@2009 initial version
 * modified by david@2012 
 */
#include <math.h>
#include "config.h"
#include "stepmotor.h"
#include "osd/osd.h"
#include "stm32f10x.h"
#include "flash.h"
#include "key.h"
#include "nvm.h"
#include "ulp_time.h"
#include "stdio.h"
#include "led.h"
#include "sys/task.h"
#include "pwm.h"
#include "l6208.h"
#include "ad9833.h"

/*sm motor status*/
enum {
	SM_IDLE,
	SM_RUNNING,
};

//private varibles
volatile static int sm_status;
volatile static int sm_stepcnt;
volatile static int sm_intcnt;
static sm_config_t sm_cfg __nvm;

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

static void sm_ExtInt(void);

void sm_Init(void)
{
	l6208_Init(&sm_cfg.l6208_cfg);    //for l6208 module init
#ifdef CONFIG_TASK_OSD
	//handle osd display
	int hdlg = osd_ConstructDialog(&sm_dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);  //set key map
#endif
	led_flash(LED_GREEN); //green led flash
	sm_ExtInt();
	sm_status = SM_IDLE;  //init for variables
	sm_stepcnt = 0;
	sm_intcnt = 0;
}

void sm_Update(void)
{

}

void EXTI9_5_IRQHandler(void)
{
	if (sm_cfg.l6208_cfg.dir)
		sm_stepcnt ++;
	else
		sm_stepcnt --;
	if (sm_cfg.runmode == SM_RUNMODE_MANUAL) {
		l6208_Stop();
		sm_status = SM_IDLE;
	} else {
		sm_intcnt ++;
		if (sm_intcnt >= sm_cfg.autosteps) {
			sm_intcnt = 0;
			l6208_Stop();
			sm_status = SM_IDLE;
		}
	}
	EXTI->PR = EXTI_Line8;
}

void main(void)
{
	task_Init();
	sm_Init();
	while(1) {
		task_Update();
		sm_Update();
	}
}

int sm_StartMotor(int clockwise)
{
	if(sm_status == SM_IDLE) {
		sm_status = SM_RUNNING;
		sm_cfg.l6208_cfg.dir = clockwise;
		if (sm_cfg.l6208_cfg.dir)
			L6208_SET_RD_CW();
		else
			L6208_SET_RD_CCW();
		l6208_Start();
	}
	return 0;
}

int sm_SetRPM(int rpm)
{
	unsigned hz;
	/*rpm min/max limit*/
	rpm = (rpm < 1) ? 1 : rpm;
	rpm = (rpm > SM_MAX_RPM) ? SM_MAX_RPM : rpm;
	hz = ((unsigned)rpm * CONFIG_STEPPERMOTOR_SPM) / 60;
	sm_cfg.rpm = rpm;
	l6208_SetClockHZ(hz);

	return 0;
}

int sm_GetRPM(void)
{
	return sm_cfg.rpm;
}

int sm_SetAutoSteps(int steps)
{
	int result = -1;
	steps = (steps < 1) ? 1 : steps;
	if(sm_status == SM_IDLE) {
		sm_cfg.autosteps = steps;
		result = 0;
	}

	return result;
}

int sm_GetAutoSteps(void)
{
	return sm_cfg.autosteps;
}

int sm_GetSteps(void)
{
	return sm_stepcnt;
}

void sm_ResetStep(void)
{
	if (sm_status == SM_IDLE) {
		sm_stepcnt = 0;
	}
}

int sm_GetRunMode(void)
{
	int mode = (sm_cfg.runmode) ? SM_RUNMODE_AUTO : SM_RUNMODE_MANUAL;
	return mode;
}

int sm_SetRunMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		sm_cfg.runmode = newmode;
	}
	return 0;
}

int sm_GetDecayMode(void)
{
	int mode = (sm_cfg.l6208_cfg.decaymode) ? SM_DECAYMODE_SLOW : SM_DECAYMODE_FAST;
	return mode;
}

int sm_SetDecayMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		sm_cfg.l6208_cfg.decaymode = newmode;
		if (sm_cfg.l6208_cfg.decaymode == SM_DECAYMODE_SLOW)
			L6208_SET_DM_SLOW();
		if (sm_cfg.l6208_cfg.decaymode == SM_DECAYMODE_FAST)
			L6208_SET_DM_FAST();
	}
	return 0;
}

int sm_GetStepMode(void)
{
	int mode = (sm_cfg.l6208_cfg.stepmode) ? SM_STEPMODE_HALF : SM_STEPMODE_FULL;
	return mode;
}

int sm_SetStepMode(int newmode)
{
	if(sm_status == SM_IDLE) {
		sm_cfg.l6208_cfg.stepmode = newmode;
		if (sm_cfg.l6208_cfg.stepmode == SM_STEPMODE_FULL)
			L6208_SET_MODE_FULL();
		if (sm_cfg.l6208_cfg.stepmode == SM_STEPMODE_HALF)
			L6208_SET_MODE_HALF();
	}
	return 0;
}

static void sm_ExtInt(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;

	/*ad9833 pulse capture irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource8);
	EXTI_InitStruct.EXTI_Line = EXTI_Line8;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_sm_info(void)
{
	printf("rpm        : %d\n", sm_cfg.rpm);
	printf("autosteps  : %d\n", sm_cfg.autosteps);

	if (sm_cfg.l6208_cfg.dir == SM_DIR_Clockwise)
	printf("dir        : ClockWise\n");
	else
	printf("dir        : CounterClockWise\n");

	if (sm_cfg.l6208_cfg.stepmode == SM_STEPMODE_FULL)
	printf("step mode  : Full Step\n");
	else
	printf("step mode  : Half Step\n");

	if (sm_cfg.l6208_cfg.decaymode == SM_DECAYMODE_FAST)
	printf("decay mode : Fast Mode\n");
	else
	printf("decay mode : Slow Mode\n");
}

static int cmd_sm_func(int argc, char *argv[])
{
	int temp;

	const char usage[] = { \
		" usage:\n" \
		" sm info,            print sm infomation, such as rpm, stepmode\n"
		" sm start cw/ccw,    set sm in auto mode and start sm run cw or ccw\n" \
		" sm as value(int),   set auto steps value\n" \
		" sm rpm value(int),  set ad9833 rpm value\n" \
		" sm mode half/full,  set half step(400/r) or full step(200/r)\n" \
		" sm decay slow/fast, set magnetic decay mode fast or slow\n" \
		" sm dir cw/ccw,      set sm directon clockwise or counterclockwise\n" \
		" sm save,            save config data\n"
	};

	if (argc == 1) {
		printf(usage);
		return 0;
	}

	if (strcmp(argv[1],"start") == 0) {
		sm_cfg.runmode = SM_RUNMODE_AUTO;
		if(strcmp(argv[2],"cw") == 0)
			sm_StartMotor(SM_DIR_Clockwise);
		if(strcmp(argv[2],"ccw") == 0)
			sm_StartMotor(SM_DIR_CounterClockwise);
	}

	if (strcmp(argv[1],"as") == 0) {
		sscanf(argv[2], "%d", &temp);
		sm_cfg.autosteps = temp;
		printf("Set OK!\n");
	}

	if (strcmp(argv[1],"rpm") == 0) {
		sscanf(argv[2], "%d", &temp);
		sm_SetRPM(temp);
		printf("Set OK!\n");
	}

	if (strcmp(argv[1],"mode") == 0) {
		if(strcmp(argv[2],"full") == 0) {
			sm_cfg.l6208_cfg.stepmode = SM_STEPMODE_FULL;
			L6208_SET_MODE_FULL();
		}
		if(strcmp(argv[2],"half") == 0) {
			sm_cfg.l6208_cfg.stepmode = SM_STEPMODE_HALF;
			L6208_SET_MODE_HALF();
		}
		printf("Set OK!\n");
	}

	if (strcmp(argv[1],"decay") == 0) {
		if(strcmp(argv[2],"slow") == 0) {
			sm_cfg.l6208_cfg.decaymode = SM_DECAYMODE_SLOW;
			L6208_SET_DM_SLOW();
		}
		if(strcmp(argv[2],"fast") == 0) {
			sm_cfg.l6208_cfg.decaymode = SM_DECAYMODE_FAST;
			L6208_SET_DM_FAST();
		}
		printf("Set OK!\n");
	}

	if (strcmp(argv[1],"dir") == 0) {
		if(strcmp(argv[2],"cw") == 0) {
			sm_cfg.l6208_cfg.dir = SM_DIR_Clockwise;
			L6208_SET_RD_CW();
		}
		if(strcmp(argv[2],"ccw") == 0) {
			sm_cfg.l6208_cfg.dir = SM_DIR_CounterClockwise;
			L6208_SET_RD_CCW();
		}
		printf("Set OK!\n");
	}

	if (strcmp(argv[1],"save") == 0) {
		nvm_save();
		printf("Save OK!\n");
	}
	
	if (strcmp(argv[1],"info") == 0) {
		print_sm_info();
	}

	return 0;
}
const cmd_t cmd_sm = {"sm", cmd_sm_func, "stepper motor debug"};
DECLARE_SHELL_CMD(cmd_sm)
#endif
