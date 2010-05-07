/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sm/stepmotor.h"
#include "osd/osd.h"
#include "stm32f10x.h"
#include "capture.h"
#include "ad9833.h"
#include "capture.h"
#include "l6208.h"
#include "lcd1602.h"
#include "spi.h"

//private varibles
static int sm_status;
static int sm_runmode;
static int sm_speed; //unit: rpm
static int sm_autosteps;

//private members for ad9833
static int sm_dds_write_reg(int reg, int val);

static ad9833_t sm_dds = {
	.io = {
		.write_reg = sm_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static int sm_dds_write_reg(int reg, int val)
{
	int ret;
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);
	ret = spi_Write(1, val);
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
	return ret;
}


void sm_Init(void)
{
	//init for ad9833
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET); // sm_dds = 1
	ad9833_Init(&sm_dds);
	
	//init for l6208
	l6208_Init();
	l6208_StartCmd(ENABLE); //enable l6208
	l6208_SelectMode(HalfMode); //enable half stepmode
	l6208_SetHomeState(ENABLE); //enable home state
	l6208_SetControlMode(DecaySlow); //enable decay slow mode
	l6208_SetRotationDirection(Clockwise); //enable clockwise dir
	
	//handle osd display
	int hdlg = osd_ConstructDialog(&sm_dlg);
	osd_SetActiveDialog(hdlg);
	
	//init for capture clock
	capture_Init();
	
	//init for variables
	sm_status = SM_IDLE;
	sm_runmode = SM_RUNMODE_MANUAL;
	sm_speed = 10;
	sm_autosteps = 1000;
}

void sm_Update(void)
{

}

void sm_StartMotor(void)
{
	sm_status = SM_RUNNING;
	capture_SetAutoRelaod(sm_autosteps);
	capture_ResetCounter(); //clear counter and preload now
	ad9833_Enable(&sm_dds);
}

void sm_StopMotor(void)
{
	sm_status = SM_IDLE;
	ad9833_Disable(&sm_dds);
}

int sm_SetSpeed(int rpm);
{
	/*rpm min/max limit*/
	rpm = (rpm < 1) ? 1 : rpm;
	
	sm_speed = rpm;
	
	/*calculate the new freqword according to the motor para,
	then setup the dds chip*/
	//ad9833_SetFreq(&sm_dds,sm_speed);
	return 0;
}

int sm_GetSpeed(void)
{
	return sm_speed;
}

int sm_SetAutoSteps(int steps);
{
	int result = -1;
	
	steps = (steps < 1) ? 1 : steps;
	
	if(sm_status == SM_IDLE) {
		sm_autosteps = steps;
		result = 0;
	}
	
	return result;
}

int sm_GetAutoSteps(void)
{
	return sm_autosteps;
}

unsigned short sm_GetSteps(void)
{
	return capture_GetCounter();
}

void sm_ResetStep(void)
{
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

void sm_isr(void)
{
	/* Clear TIM1 Update interrupt pending bit */
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

}

