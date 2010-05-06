/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sm/stepmotor.h"
#include "osd/osd.h"
#include "stm32f10x.h"
#include "capture.h"
#include "ad9833.h"
#include "l6208.h"
#include "lcd1602.h"

//private members define
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
	ad9833_Init(&rpm_dds);
	
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

void sm_isr(void)
{
	/* Clear TIM1 Update interrupt pending bit */
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

}


