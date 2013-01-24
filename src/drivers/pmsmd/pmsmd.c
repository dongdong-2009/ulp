/*
 * 	king@2013 initial version
 */

#include "stm32f10x.h"
#include "./pmsmd.h"

static const int sector_decide[8] = {0, 2, 6, 1, 4, 3, 5, 0};

static int sector_now;

static svm_dev_t dev = {

	.motor.pp = 2,

	.encoder.crp = 400,

	.current.bias_a = 0,
	.current.bias_b = 0,
	.current.bias_c = 0,
	.current.bias_total = 0,

	.current.ra = 0,
	.current.rb = 0,
	.current.rc = 0,

	.svpwm.deadtime = 100,

};

	/*
	 * 	TIMER3
	 *
	 * 	PC6:	CH1	ENCODE_A
	 * 	PC7:	CH2	ENCODE_B
	 *
	 * 	PC8:	CH3	ENCODE_I(Reserved)
	 */

static void encoder_init()
{

	dev.encoder.crp_multi_4 = dev.encoder.crp << 2;
	dev.encoder.crp_multi_2 = dev.encoder.crp << 1;

	/* 	enable clock	 */

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);

	/*
	 * 	initialize TIMER3 GPIO
	 *
	 * 	PC6:	CH1	ENCODE_A
	 * 	PC7:	CH2	ENCODE_B
	 *
	 * 	PC8:	CH3	ENCODE_I(Reserved)
	 */

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;// | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 	config TIMER3	 */

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;

	/* Timer configuration in Encoder mode */
	TIM_DeInit(TIM3);
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

	TIM_TimeBaseStructure.TIM_Period = dev.encoder.crp_multi_4 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x00;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, DISABLE);

	dev.encoder.angle_per_counter_q10 = (0x00010000 << 10) / dev.encoder.crp_multi_4;
}

static void angle_get(int *angle_elec, int *angle_mech)
{
	int a = dev.encoder.angle_per_counter_q10;
	int b = (int)TIM3->CNT;
	int c;
	a *= b;
	a = a >> 10;
	c = a - 0x00008000;
	*angle_mech = c;
	c += 0x000100000;
	*angle_elec = ((c * dev.motor.pp + 0x00008000) & 0x0000ffff) - 0x00008000;
}

static void angle_set(int angle_mech)
{
	TIM_SetCounter(TIM3, (unsigned short)(((angle_mech + 0x00008000) << 10) / dev.encoder.angle_per_counter_q10));
}

static void encoder_ctl(int enable)
{
	if(enable) {
		TIM_Cmd(TIM3, ENABLE);
	}
	else {
		TIM_Cmd(TIM3, DISABLE);
	}
}

static void svpwm_init(int frq)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE ,ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET);

	/* 	enable clock	 */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1,ENABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);

	/*
	 * 	initialize TIMER1 GPIO
	 *
	 * 	PE9:	CH1
	 * 	PE11:	CH2
	 * 	PE13:	CH3
	 * 	PE8:	CH1N
	 * 	PE10:	CH2N
	 * 	PE12:	CH3N
	 * 	PE15:	BREAK
	 *
	 * 	PE14:	CH4
	 */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* 	config NVIC	 */

	NVIC_InitTypeDef NVIC_InitStructure;

#ifdef VECT_TAB_RAM
	NVIC_SetVectorTable(NVIC_VectTab_RAM,0x0);
#else
	NVIC_SetVectorTable(NVIC_VectTab_FLASH,0x0);
#endif

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* 	config TIMER1	 */

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	dev.svpwm.frq = frq;
	dev.svpwm.tpwm = RCC_Clocks.PCLK2_Frequency / dev.svpwm.frq;
	dev.svpwm.tpwm_div_2 = dev.svpwm.tpwm >> 1;
	TIM_DeInit(TIM1);

	TIM_TimeBaseStructure.TIM_Period = dev.svpwm.tpwm_div_2;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM1, 0, TIM_PSCReloadMode_Immediate);
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);

	/* 	set adc sample point(cased by TIM1->CC4)	 */
	TIM_OCInitStructure.TIM_Pulse = dev.svpwm.tpwm_div_2 - 1;
	TIM_OC4Init(TIM1, &TIM_OCInitStructure);

	TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
	TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
	TIM_BDTRInitStructure.TIM_DeadTime = dev.svpwm.deadtime;
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable;
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_GenerateEvent(TIM1, TIM_EventSource_Update);

	//TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);
	TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);

	TIM_Cmd(TIM1, DISABLE);
	TIM_CtrlPWMOutputs(TIM1, DISABLE);

	dev.svpwm.tpwm_div_2_multi_sqrt3 = (0x00006ED9 * (int)dev.svpwm.tpwm_div_2) >> 14;//	sqrt(3) = 0x00006ED9(Q14)
}

/*
 * 	alpha:per-unit(Q15)
 * 	beta:per-unit(Q15)
 */

static void svpwm_set(int alpha, int beta)
{
	int u1, u2, u3;
	int temp;
	unsigned short t[8], ch[3];	//t[i] = T[i]/2

	/*
	 * 	u1 = beta
	 * 	u2 = sqrt(3)*alpha/2 - beta/2
	 * 	u3 = -sqrt(3)*alpha/2 - beta/2
	 *
	 * 	sqrt(3)/2 = 0.86602540378444 = 0x00006ED9(Q15)
	 */

	u1 = beta;
	u2 = ((0x00006ED9 * alpha) >> 15) - (beta >> 1);
	u3 = ((-0x00006ED9 * alpha) >> 15) - (beta >> 1);

	/* 	decide the sector	 */

	temp = u1 >= 0 ? 1 : 0;
	temp |= u2 >= 0 ? 2 : 0;
	temp |= u3 >= 0 ? 4 : 0;
	sector_now = sector_decide[temp];

	/*
	 * 	calculate time and config timer1 registers
	 */

	switch(sector_now) {
	case 0:
		ch[0] = (unsigned short)dev.svpwm.tpwm_div_2;
		ch[1] = (unsigned short)dev.svpwm.tpwm_div_2;
		ch[2] = (unsigned short)dev.svpwm.tpwm_div_2;
		break;
	case 1:
		t[4] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u2) >> 15);
		t[6] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u1) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[4] - t[6]) >> 1;
		ch[2] = t[7];
		ch[1] = ch[2] + t[6];
		ch[0] = ch[1] + t[4];
		break;
	case 2:
		t[2] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u2) >> 15);
		t[6] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u3) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[2] - t[6]) >> 1;
		ch[2] = t[7];
		ch[0] = ch[2] + t[6];
		ch[1] = ch[0] + t[2];
		break;
	case 3:
		t[2] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u1) >> 15);
		t[3] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u3) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[2] - t[3]) >> 1;
		ch[0] = t[7];
		ch[2] = ch[0] + t[3];
		ch[1] = ch[2] + t[2];
		break;
	case 4:
		t[1] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u1) >> 15);
		t[3] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u2) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[1] - t[3]) >> 1;
		ch[0] = t[7];
		ch[1] = ch[0] + t[3];
		ch[2] = ch[1] + t[1];
		break;
	case 5:
		t[1] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u3) >> 15);
		t[5] = (unsigned short)((dev.svpwm.tpwm_div_2_multi_sqrt3 * u2) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[1] - t[5]) >> 1;
		ch[1] = t[7];
		ch[0] = ch[1] + t[5];
		ch[2] = ch[0] + t[1];
		break;
	case 6:
		t[4] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u3) >> 15);
		t[5] = (unsigned short)((-dev.svpwm.tpwm_div_2_multi_sqrt3 * u1) >> 15);
		t[7] = (dev.svpwm.tpwm_div_2 - t[4] - t[5]) >> 1;
		ch[1] = t[7];
		ch[2] = ch[1] + t[5];
		ch[0] = ch[2] + t[4];
		break;
	case 7:
		ch[0] = (unsigned short)dev.svpwm.tpwm_div_2;
		ch[1] = (unsigned short)dev.svpwm.tpwm_div_2;
		ch[2] = (unsigned short)dev.svpwm.tpwm_div_2;
		break;
	}
		TIM_SetCompare1(TIM1, ch[0]);
		TIM_SetCompare2(TIM1, ch[1]);
		TIM_SetCompare3(TIM1, ch[2]);
}

static void svpwm_crl(int enable)
{
	if(enable) {
		GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET);
		TIM_Cmd(TIM1, ENABLE);
		TIM_CtrlPWMOutputs(TIM1, ENABLE);
	}
	else {
		GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET);
		TIM_Cmd(TIM1, DISABLE);
		TIM_CtrlPWMOutputs(TIM1, DISABLE);
	}
}

static void svpwm_isr_ctl(int enable)
{
	if(enable) {
		TIM_ITConfig(TIM1, TIM_IT_CC4, ENABLE);
	}
	else {
		TIM_ITConfig(TIM1, TIM_IT_CC4, DISABLE);
	}
}

static void current_feedback_init()
{
	/* 	enable clock	 */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	/*
	 *	initialize ADC1 GPIO
	 *	PC0:	ADC_IN10	Ia
	 *	PC1:	ADC_IN11	Ib
	 *	PC2:	ADC_IN12	Ic
	 *	PC3:	ADC_IN13	Motor Supply
	  *	PC4:	ADC_IN14	Itotal
	 */

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* 	config ADC1	 */

	ADC_InitTypeDef ADC_InitStructure;

	ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 4);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_14, 4, ADC_SampleTime_7Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);

	/* 	Calibration	 */
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

}

static void current_get(int *pIa, int *pIb, int *pIc, int *pItotal)
{
	while(!(ADC1->SR & 0x00000004)); //wait for the end of conversion

	*pIa = ADC1->JDR1 - dev.current.bias_a;
	*pIb = ADC1->JDR2 - dev.current.bias_b;
	*pIc = ADC1->JDR3 - dev.current.bias_c;
	*pItotal = ADC1->JDR4 - dev.current.bias_total;

	ADC1->SR |= 0xfffffffb;
}

static void svm_init(int frq)
{
	encoder_init();
	svpwm_init(frq);
	current_feedback_init();
}

svm_func_t svm1_stm32 = {

	.init = svm_init,

	.current_get = current_get,
	.position_set = angle_set,
	.position_ctl = encoder_ctl,
	.position_get = angle_get,

	.svpwm_set = svpwm_set,
	.svpwm_ctl = svpwm_crl,

	.current_get = current_get,

	.isr_ctl = svpwm_isr_ctl,
};
