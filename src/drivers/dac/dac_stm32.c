/*
 *	david@2010 initial version
 */

#include "stm32f10x.h"
#include "config.h"
#include "dac.h"
#include <string.h>
#include <assert.h>

static int dac_ch1_init(const dac_cfg_t * cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitStructure;

	/* GPIOA Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* DAC Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	/* Once the DAC channel is enabled, the corresponding GPIO pin is automatically
	connected to the DAC converter. In order to avoid parasitic consumption,
	the GPIO pin should be configured in analog */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* DAC channel1 Configuration */
	DAC_StructInit(&DAC_InitStructure);
        
        //DAC_InitStructure.DAC_Trigger=DAC_Trigger_Software;//由软件触发
        //DAC_InitStructure.DAC_WaveGeneration=DAC_WaveGeneration_None;//关闭波形生成
        //DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude=DAC_TriangleAmplitude_4095;
        //DAC_InitStructure.DAC_OutputBuffer=DAC_OutputBuffer_Enable;//使能DAC通道缓存
        
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);

	/* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is
	automatically connected to the DAC converter. */
	DAC_Cmd(DAC_Channel_1, ENABLE);

	return 0;
}

static int dac_ch1_write(int data)
{
	DAC_SetChannel1Data(DAC_Align_12b_R, data);

	return 0;
}

static int dac_ch2_init(const dac_cfg_t * cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitStructure;

	/* GPIOA Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* DAC Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	/* Once the DAC channel is enabled, the corresponding GPIO pin is automatically
	connected to the DAC converter. In order to avoid parasitic consumption,
	the GPIO pin should be configured in analog */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* DAC channel1 Configuration */
	DAC_StructInit(&DAC_InitStructure);
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	/* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is
	automatically connected to the DAC converter. */
	DAC_Cmd(DAC_Channel_2, ENABLE);

	return 0;
}

static int dac_ch2_write(int data)
{
	DAC_SetChannel2Data(DAC_Align_12b_R, data);

	return 0;
}

const dac_t dac_ch1 = {
	.init = dac_ch1_init,
	.write = dac_ch1_write,
};

const dac_t dac_ch2 = {
	.init = dac_ch2_init,
	.write = dac_ch2_write,
};


#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_dac_func(int argc, char *argv[])
{
	const char * usage = { \
		" usage:\n" \
		" dac init chx, init the stm32 dac chx(ch1 or ch2)\n" \
		" dac write chx data, write data to the dac chx(ch1 or ch2)\n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (!strcmp(argv[1], "init")) {
		if (argv[2][2] == '1')
			dac_ch1.init(NULL);
		if (argv[2][2] == '2')
			dac_ch2.init(NULL);
	}

	if (!strcmp(argv[1], "write")) {
		if (argv[2][2] == '1') {
			dac_ch1.write(atoi(argv[3]));
		}
		if (argv[2][2] == '2') {
			dac_ch2.write(atoi(argv[3]));
		}
	}

	return 0;
}

const cmd_t cmd_dac = {"dac", cmd_dac_func, "dac cmd"};
DECLARE_SHELL_CMD(cmd_dac)
#endif
