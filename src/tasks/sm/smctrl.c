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

static ad9833_t sm_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB10,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

void smctrl_Init(void)
{
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


