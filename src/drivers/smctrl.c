/*
*	dusk@2010 initial version
*/

#include "config.h"
#include "smctrl.h"
#include "time.h"
#include "l6208.h"
#include "ad9833.h"
#include "spi.h"
#include "smpwm.h"
#include <string.h>

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
	GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_RESET);
	ret = spi_Write(1, val);
	GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_SET);
	return ret;
}

void smctrl_Init(void)
{
	//init for ad9833
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOB, GPIO_Pin_10, Bit_SET); // sm_dds = 1
	ad9833_Init(&sm_dds);
	
	//init for l6208
	l6208_Init();
	l6208_StartCmd(ENABLE); //enable l6208
	l6208_SetHomeState(); //enable home state
	
#ifdef CONFIG_STEPPERMOTOR_HALFSTEP
	l6208_SelectMode(HalfMode); //enable half stepmode
#else
	l6208_SelectMode(FullMode); //enable half stepmode
#endif

#ifdef CONFIG_STEPPERMOTOR_FASTDECAYSTEP
	l6208_SetControlMode(DecayFast); //enable decay slow mode
#else
	l6208_SetControlMode(DecaySlow); //enable decay slow mode
#endif

	//init for sm pwm
	smpwm_Init();
	smpwm_SetDuty(50); //duty is 50%

#if CONFIG_STEPPERMOTOR_IDLE_POWEROFF == 1
	l6208_StartCmd(DISABLE);
#endif
}

void smctrl_SetRPM(int rpm)
{
	/*calculate the new freqword according to the motor para,
	then setup the dds chip*/
	unsigned hz, mclk, fw;
	
	hz = ((unsigned)rpm * CONFIG_STEPPERMOTOR_SPM) / 60;
	mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	fw = hz;	
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&sm_dds,fw);
}

void smctrl_SetRotationDirection(int dir)
{
	if(dir)
		l6208_SetRotationDirection(CounterClockwise);
	else
		l6208_SetRotationDirection(Clockwise);
}

void smctrl_Start(void)
{
#if CONFIG_STEPPERMOTOR_IDLE_POWEROFF == 1
	l6208_StartCmd(ENABLE);
#endif
	ad9833_Enable(&sm_dds);
}

void smctrl_Stop(void)
{
	ad9833_Disable(&sm_dds);
#if CONFIG_STEPPERMOTOR_IDLE_POWEROFF == 1
	l6208_StartCmd(DISABLE);
#endif
}


