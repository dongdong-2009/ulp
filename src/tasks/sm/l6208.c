/*
*	dusk@2010 initial version
*/
#include <string.h>
#include "l6208.h"
#include "ulp_time.h"
#include "ad9833.h"
#include "pwm.h"

static ad9833_t sm_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB10,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

void l6208_Init(const l6208_config_t * config)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOB clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* Configure PB as output push pull*/
	GPIO_InitStructure.GPIO_Pin = L6208_RST | L6208_ENA | L6208_DECAY | L6208_HoF | L6208_DIR;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

#ifdef L6208_VREF_USE_PWM
	//pwm module init, used for L6208 over current protect
	pwm_cfg_t cfg;
	cfg.hz = 2000;
	cfg.fs = 100;
	pwm33.init(&cfg);
	pwm33.set(50);
#endif

	if (config->dir == SM_DIR_Clockwise)
		L6208_SET_RD_CW();
	else
		L6208_SET_RD_CCW();
	if (config->stepmode == SM_STEPMODE_HALF)
		L6208_SET_MODE_HALF();
	else
		L6208_SET_MODE_FULL();
	if (config->decaymode == SM_DECAYMODE_FAST)
		L6208_SET_DM_FAST();
	else
		L6208_SET_DM_SLOW();
	L6208_ENABLE();
	l6208_SetHomeState();
	L6208_DISABLE();
	ad9833_Init(&sm_dds);
}

/*
 * set the stepper motor to home state
 */
void l6208_SetHomeState(void)
{
	int us = 10;
	GPIO_ResetBits(GPIOB,L6208_RST);
	while(us > 0) us--;
	GPIO_SetBits(GPIOB,L6208_RST);
}

/*
 * set the stepper motor input state machine clock
 */
void l6208_SetClockHZ(int hz)
{
	unsigned mclk, fw;

	mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&sm_dds,fw);
	ad9833_Disable(&sm_dds);
}

void l6208_Start(void)
{
	L6208_ENABLE();
	ad9833_Enable(&sm_dds);
}

void l6208_Stop(void)
{
	L6208_DISABLE();
	ad9833_Disable(&sm_dds);
}
