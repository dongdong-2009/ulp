/*
*
*  miaofng@2014-6-4   initial version
*
*/
#include "ulp/sys.h"
#include "../irc.h"
#include "../err.h"
#include "stm32f10x.h"
#include "can.h"
#include "dps.h"

#define PWM_BITS 16
#define TPS_VREF 2.50 //unit: V
#define TPS_GFB (50*6/20) //1/50
#define IS_VREF 3.30
#define IS_RSHUNT 0.050 //unit: ohm

/*ADC DMA FIFO*/
enum {
	DPS_FIS,
	DPS_FHV,
	DPS_FSZ,
};
static unsigned short dps_fifo[DPS_FSZ]; //ISFB HVFB
static char dps_gsel;
static const can_bus_t *dps_can = &can1;

static void dps_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/*pinmap
	PSEN	PA2	DIN
	PSGD	PA3	PUSH-PULL

	LV_EN	PB12	PUSH-PULL
	HV_GS	PB10	PUSH-PULL
	HV_EN	PB11	PUSH-PULL
	IS_GS0	PB0	PUSH-PULL
	IS_GS1	PB1	PUSH-PULL

	IS_FB	PC2/AI12	AIN
	HV_FB	PC3/AI13	AIN

	LV_PWM	PB6	PUSH-PULL
	IS_PWM	PB7	PUSH-PULL
	HS_PWM	PB8	PUSH-PULL
	HV_PWM	PB9	PUSH-PULL
	*/

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*TIM4, note:
1, TIMXCLK clock is derived from PCLK1(36 MHz at most)
2, TIMXCLK = 72MHz, why ???
3, 72MHz/65536 = 1.098KHz (Fpwm)
*/
#define PWM_MAX ((1 << 16) -1)
#define PWM_DIV 1

static void dps_pwm_init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = PWM_MAX;
	TIM_TimeBaseStructure.TIM_Prescaler = PWM_DIV - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_Cmd(TIM4, ENABLE);
}

/* sw trig once a systick
IS_FB	PC2/AI12	AIN -> ADC1
HV_FB	PC3/AI13	AIN
*/
static void dps_adc_init(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); /*72Mhz/8 = 9Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//ADC_ExternalTrigConv_Ext_IT11_TIM8_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, DPS_FIS + 1, ADC_SampleTime_239Cycles5); //9Mhz/(239.5 + 12.5) = 35.7Khz
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, DPS_FHV + 1, ADC_SampleTime_239Cycles5); //9Mhz/(239.5 + 12.5) = 35.7Khz
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_Cmd(ADC1, DISABLE);

	//setup dma
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef  DMA_InitStructure;
	DMA_DeInit(DMA1_Channel1);
	DMA_Cmd(DMA1_Channel1, DISABLE);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (unsigned)&ADC1 -> DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (unsigned) dps_fifo;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = DPS_FSZ;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
}

static void dps_can_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	dps_can->init(&cfg);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

//LV_EN	PB12	PUSH-PULL
void lv_enable(int yes)
{
	if(yes) {
		GPIOB->BSRR = 1 << 12;
	}
	else {
		GPIOB->BRR = 1 << 12;
	}
}

/*TPS40170 LV_PWM TIM4_CH1 PB6
1, LV range = 0 .. 24V(30v)
2, Vtrack <= 600mV
3, Gfb = 1/50
4, Vref = 2.5v, best choice is 1.024v, but hard to purchase
*/
void lv_u_set(float v)
{
	/* vout = 50 * 2.5V * n / 65536
	=> n = vout / 50 / 2.5V * 65536*/

	static const float gain = (1 << PWM_BITS) / TPS_GFB / TPS_VREF;
	int d = (int)( v * gain);
	d = (d > PWM_MAX) ? PWM_MAX : d;
	TIM4->CCR1 = d & 0xffff;
}

/* IS_PWM PB7 TIM4_CH2, note:
1, INA225 gain option = 200 100 50 25
2, Vsense = Iout * Rshunt * Gis
=> Iout = Vsense / Rshunt / Gis = (n / 65536 * 3.3v) / Rshunt / Gis
=> n = Iout * Rshunt * Gis * 65536 / 3.3
*/
void is_i_set(float a)
{
	static const float gain = (IS_RSHUNT * 200 * ((1 << PWM_BITS) / IS_VREF));
	int d = (int) (a * gain);

	/*now we got 4 different value towards 4 different gain settings, which one is best???
	upper limit: vref->3.3v, LMV762->3.8v, ina225->3.3v-0.1v=3.2v
	lower limit: vref->0v, LMV762->-0.3v, ina225->0.1v
	*/
	const int max = (int)(3.2 / IS_VREF * (1 << PWM_BITS)); //3.2v
	const int min = (int)(0.1 / IS_VREF * (1 << PWM_BITS)); //0.1v
	int sumsq_min = 0;
	for(int gsel = 0; gsel < 4; gsel ++) {
		int y = d >> gsel;
		int sumsq = (max - y) * (max - y) + (y - min) * (y - min);
		if((sumsq_min == 0) || (sumsq < sumsq_min)) {
			dps_gsel = gsel;
			sumsq_min = sumsq;
		}
	}

	/*ok, we found the best gsel*/
	d >>= dps_gsel;
	d = (d > PWM_MAX) ? PWM_MAX : d;

	/* IS_GS1(PB1) IS_GS0(PB0) */
	int port = GPIOB->ODR;
	port &= ~0x03;
	port |= dps_gsel & 0x03;
	GPIOB->ODR = port;
	TIM4->CCR2 = d;
}

/*
2, Vsense = Iout * Rshunt * Gis
=> Iout = Vsense / Rshunt / Gis = (n / 65536 * 3.3v) / Rshunt / Gis
=> n = Iout * Rshunt * Gis * 65536 / 3.3
=> Iout = n / (Rshunt * Gis * 65536 / 3.3) = n * (3.3 / (Rshunt * Gis * 65536))
*/
float is_i_get(void)
{
	#define ADC_BITS 15
	static const float gain = IS_VREF / (IS_RSHUNT * 25 * (1 << ADC_BITS));
	int d = dps_fifo[DPS_FIS] >> dps_gsel;
	float amp = d * gain;
	return amp;
}

void dps_drv_init(void)
{
	dps_gpio_init();
	dps_pwm_init();
	dps_adc_init();
	dps_can_init();
}

void __sys_tick(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void ADC1_2_IRQHandler(void)
{
	//ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
	ADC1->SR &= ~ (1 << 1);
	dps_auto_regulate();
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	can_msg_t msg;
	while(!dps_can->recv(&msg)) {
		dps_can_handler(&msg);
	}
}
