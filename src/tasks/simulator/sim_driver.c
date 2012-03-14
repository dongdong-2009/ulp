/*
 * david@2012 initial version
 */
#include <stdio.h>
#include <stdlib.h>
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

void clock_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&clock_dds, fw);
}

void axle_Init(int option)  //such as 58x, 38x
{
	dac_ch1.init(NULL);
}

void axle_SetFreq(short hz)
{
	clock_SetFreq(hz >> 4);
}

void axle_SetAmp(short amp)
{
	dac_ch1.write(amp);
}

void op_Init(int option)  //such as 58x, 38x
{
	dac_ch2.init(NULL);
}

void op_SetAmp(short amp)
{
	dac_ch2.write(amp);
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
