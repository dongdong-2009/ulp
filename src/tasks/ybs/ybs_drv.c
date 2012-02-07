/*
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */
#include "config.h"
#include "ybs.h"
#include "ybs_drv.h"
#include "ulp/dac.h"
#include "ulp_time.h"
#include "spi.h"
#include "stm32f10x.h"

static int ybs_fd_dac = 0;

int ybs_drv_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	//mos ctrl, PB0, high off, low on
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//adc, PA1, ADC12_IN1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_71Cycles5); //9Mhz/(71.5 + 12.5) = 107.1Khz
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	//TIM4 TIMEBASE
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseStructure.TIM_Period = YBS_US - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; //Fapb1 = TIM_clk = 72Mhz, Tick = 1us
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	//TIM_ARRPreloadConfig(TIM4, ENABLE);

	TIM_Cmd(TIM4, DISABLE);
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	//IRQ OF TIM4
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//DAC
	static const struct ad5663_cfg_s ad5663_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ldac = SPI_CS_PA2,
		.gpio_clr = SPI_CS_PA3,
	};
	dev_register("ad5663", &ad5663_cfg);
	ybs_fd_dac = dev_open("dac0", 0);
	dev_ioctl(ybs_fd_dac, DAC_SET_CH, 1); //for fast ch1 write purpose
	return 0;
}

void TIM4_IRQHandler(void)
{
	ybs_update();
	TIM4->SR = ~TIM_IT_Update;
}

void ybs_tim_set(int on)
{
	FunctionalState s = (on) ? ENABLE : DISABLE;
	TIM_Cmd(TIM4, s);
}

void ybs_mos_set(int on)
{
	BitAction ba;
	ba = (on) ? Bit_RESET : Bit_SET;
	GPIO_WriteBit(GPIOB, GPIO_Pin_0, ba);
}

int ybs_set_vr(int d) //set Vref
{
	int uv_per_bit = (int)(VREF_DAC * 1e6 / (1 << 16));
	d = d2uv(d) / uv_per_bit;
	d = (d > 0xffff) ? 0xffff : d;
	d = (d < 0) ? 0 : d;
	dev_ioctl(ybs_fd_dac, DAC_SET_CH, 0);
	dev_write(ybs_fd_dac, &d, 0);
	dev_ioctl(ybs_fd_dac, DAC_SET_CH, 1); //for fast ch1 write purpose
	return 0;
}

int ybs_set_vo(int d)
{
	int uv_per_bit = (int)(VREF_DAC * 1e6 / (1 << 16));
	d = d2uv(d) / uv_per_bit;
	d = (d > 0xffff) ? 0xffff : d;
	d = (d < 0) ? 0 : d;
	dev_write(ybs_fd_dac, &d, 0);
	return 0;
}

int ybs_get_vi(void)
{
	while(ADC1->SR & ADC_FLAG_EOC == 0);
	int d = ADC1->DR;
	ADC1->SR &= ~ADC_FLAG_EOC;
	ADC1->CR2 |= 0x00500000; //CR2_EXTTRIG_SWSTART_Set;

	int uv_per_bit = (int)(VREF_ADC * 1e6 / (1 << 12));
	d = uv2d(d * uv_per_bit);
	return d;
}

#define _bn 19
#define _an 14
static struct filter_s ybs_vi_filter = {
	.bn = _bn,
	.b0 = (int)(0.002931688216512629/*0.0006279240770159511*/ * (1 << _bn)),
	.b1 = (int)(0.002931688216512629/*0.0006279240770159511*/ * (1 << _bn)),
	.an = _an,
	.a0 = (int)(1.0000000000000000000 * (1 << _an)),
	.a1 = (int)(-0.99413662356697474 /*-0.9987441518459681*/ * (1 << _an)),

	.xn_1 = 0,
	.yn_1 = 0,
};

int ybs_get_vi_mean(void)
{
	int i, v, vf = 0;
	ybs_vi_filter.xn_1 = 0;
	ybs_vi_filter.yn_1 = 0;
	ybs_get_vi(); //old data, ignore
	for(i = 0; i < 2000; i ++) {
		//v += ybs_get_vi();
		//v >>= (i == 0) ? 0 : 1;
		v = ybs_get_vi();
		vf = v >> 6;
		vf = filt(&ybs_vi_filter, vf);

		//printf("vori = %d mv, vfil = %d mv\n", d2mv(v), d2mv(vf << 6));
	}
	vf <<= 6;
	return vf;
}


