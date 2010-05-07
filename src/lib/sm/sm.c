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

//private varibles for run mode
static int sm_runmode; //0 = manual, 1 = auto
const char runmode_manual[] = "manual";
const char runmode_auto[] = "auto";

//private members for auto steps
static unsigned short sm_autosteps = 1000;

//private members for rpm
static int sm_speed = 10; //10hz


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
	
	//init for lcd1602
	lcd1602_Init();
	
	//handle osd display
	int hdlg = osd_ConstructDialog(&sm_dlg);
	osd_SetActiveDialog(hdlg);
	
	//init for capture clock
	capture_Init();
	
	//init for variables
	sm_runmode = 0;
}

void sm_Update(void)
{

}

void sm_StartMotor(void)
{
	ad9833_Enable(&sm_dds);
}

void sm_StopMotor(void)
{
	ad9833_Disable(&sm_dds);
}

void sm_SetSpeed(sm_rpm_t sm_rpm)
{
	switch(sm_rpm){
	case RPM_DEC:
		sm_speed--;
		break;
	case RPM_INC:
		sm_speed++;
		break;
	case RPM_RESET:
		sm_speed = 1000;
		break;
	case RPM_OK:
		ad9833_SetFreq(&sm_dds,sm_speed);
		break;
	default:
		break;
	}
}

int sm_GetSpeed(void)
{
	return sm_speed;
}

void sm_SetAutoSteps(sm_autostep_t autostep)
{
	switch(autostep){
	case STEP_DEC:
		if(sm_autosteps>0)
			sm_autosteps--;
		else
			sm_autosteps = 0;
		break;
	case STEP_INC:
		if(sm_autosteps<65535)
			sm_autosteps++;
		else
			sm_autosteps = 65535;
		break;
	case STEP_RESET:
		sm_autosteps = 1000;
		break;
	case STEP_OK:
		capture_SetAutoRelaod(sm_autosteps);
		capture_ResetCounter(); //clear counter and preload now
		break;
	default:
		break;
	}
}

unsigned short sm_GetSteps(void)
{
	return capture_GetCounter();
}

void sm_ResetStep(void)
{
	capture_ResetCounter();
}

void sm_ResetAutoSteps(void)
{
	capture_ResetCounter();
}

unsigned short sm_GetAutoSteps(void)
{
	return capture_GetAutoReload();
}

int sm_GetRunMode(void)
{
	if(sm_runmode)
		return (int)runmode_auto;
	else
		return (int)runmode_manual;
}

void sm_ChangeRunMode(void)
{
	sm_runmode = !sm_runmode;
}

void sm_isr(void)
{
	/* Clear TIM1 Update interrupt pending bit */
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

}

