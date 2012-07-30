/*
 * david@2012 initial version
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "priv/mcamos.h"
#include "ad9833.h"
#include "mcp41x.h"
#include "nvm.h"
#include "dac.h"
#include "sim_driver.h"
#include "pwm.h"
#include "pwmin.h"
#include "counter.h"

#define SIMULATOR_DEBUG 1
#if SIMULATOR_DEBUG
#define DEBUG_TRACE(args)    (printf args)
#else
#define DEBUG_TRACE(args)
#endif


static ad9833_t clock_dds = {
	.bus = &spi2,
	.idx = SPI_CS_PB12,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t vss_dds = {
	.bus = &spi2,
	.idx = SPI_CS_PC7, //real one
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t wss_dds = {
	.bus = &spi2,
	.idx = SPI_CS_PB1,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

void driver_Init(void)
{

}

void clock_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;

	/*rpm irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	clock_Enable(0);/*misfire input switch*/

	ad9833_Init(&clock_dds);
}

void clock_SetFreq(int hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	int64_t fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&clock_dds, (unsigned)fw);
}

void axle_Init(int option)  //such as 58x, 38x
{
	dac_ch1.init(NULL);
}

void axle_SetFreq(int hz)
{
	clock_SetFreq(hz << 4);
}

void op_Init(int option)  //such as 58x, 38x
{
	dac_ch2.init(NULL);
}

void clock_Enable(int on)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = (on > 0) ? ENABLE : DISABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void vss_Init(void)
{
	ad9833_Init(&vss_dds);
}

void wss_Init(void)
{
	ad9833_Init(&wss_dds);
}

void vss_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&vss_dds, fw);
}

void wss_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&wss_dds, fw);
}

void counter1_Init(void)
{
	counter11.init(NULL);
}

void counter2_Init(void)
{
	counter22.init(NULL);
}

int counter1_GetValue(void)
{
	return counter11.get();
}

int counter2_GetValue(void)
{
	return counter22.get();
}

void counter1_SetValue(int value)
{
	counter11.set(value);
}

void counter2_SetValue(int value)
{
	counter22.set(value);
}

//pwm1 : TIM3_CH3
void pwm1_Init(int frq, int dc)
{
	pwm_cfg_t cfg;
	const pwm_bus_t *pwm = &pwm33;

	cfg.hz = frq;
	cfg.fs = 100;

	pwm->init(&cfg);
	pwm->set(dc);
}

//pwm2 : TIM4_CH2
void pwm2_Init(int frq, int dc)
{
	pwm_cfg_t cfg;
	const pwm_bus_t *pwm = &pwm42;

	cfg.hz = frq;
	cfg.fs = 100;

	pwm->init(&cfg);
	pwm->set(dc);
}

void pwmin1_Init(void)
{
	pwmin_cfg_t cfg;
	cfg.sample_clock = 1000000;
	pwmin11.init(&cfg);
}

int pwmin1_GetDC(void)
{
	return pwmin11.getdc();
}

int pwmin1_GetFrq(void)
{
	return pwmin11.getfrq();
}


