/* vsm.c
*	dusk@2009 initial version
*	miaofng@2009
*/

#include "stm32f10x.h"
#include "vsm.h"
#include "normalize.h"

static int sector;
static short vp;

/*ADC channel map*/
#define VSM_PHU	ADC_Channel_4 //PA4
#define VSM_PHV	ADC_Channel_5 //PA5
#define VSM_PHW	ADC_Channel_14 //PC4
#define VSM_VDC	ADC_Channel_11 //PC1
#define VSM_TOT	ADC_Channel_15 //PC5

void vsm_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	TIM_BDTRInitTypeDef TIM_BDTRInitStructure;

	/*step 0, clk configuration*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	/*step 1, pwm1 init*/
	/* pin map:
		PWM_UP	PA8
		PWM_VP	PA9
		PWM_WP	PA10
		PWM_UN	PB13
		PWM_VN	PB14
		PWM_WN	PB15
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;//|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//GPIO_PinLockConfig(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	//GPIO_PinLockConfig(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);

	TIM_TimeBaseStructure.TIM_Period = T;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //CKD = 0, Tdts = Tck_int = 72Mhz?
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 1;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
	TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
	TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF;
	TIM_BDTRInitStructure.TIM_DeadTime = TD*72/1000;
	TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
	TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_Pulse = T;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);
	TIM_OC4Init(TIM1, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	TIM_Cmd(TIM1, ENABLE);

	/*step 2, adc1/2 init*/
	/* pin map:
		ADC_U	PA4/IN4
		ADC_V	PA5/IN5
		ADC_W	PC4/IN14
		ADC_VDC	PC1/IN11
		ADC_TOTAL	PC5/IN15
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_DeInit(ADC2);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, VSM_VDC, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, VSM_PHU, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, VSM_PHV, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC1, VSM_PHW, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC2, VSM_PHU, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC2, VSM_PHV, 1, ADC_SampleTime_7Cycles5);
	ADC_InjectedChannelConfig(ADC2, VSM_PHW, 1, ADC_SampleTime_7Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC2, ENABLE);

	ADC_Cmd(ADC1, ENABLE);
	ADC_Cmd(ADC2, ENABLE);

	ADC_StartCalibration(ADC1);
	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC1) & ADC_GetCalibrationStatus(ADC2));

	/*step 3, adc irq init*/
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

#define REG_ADC_BITS (16) //left aligh, msb is sign bit
#define MV_PER_CNT (VSM_VREF_MV / (1 << REG_ADC_BITS))
#define CNT_2_VDC(cnt) ((cnt * (int) (MV_PER_CNT / VSM_VDC_RATIO * (1 << 12))) >> 12)

void vsm_Update(void)
{
	int cnt, vdc, tmp;

	/*get bus/phase voltage*/
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	cnt = ADC_GetConversionValue(ADC1);

	vdc = CNT_2_VDC(cnt);
	tmp = NOR_VOL(vdc);
	tmp = tmp * divSQRT_3; /*vp = vb / (3^0.5)*/
	vp = tmp >> 15;
}

/* config the duty cycle */
void vsm_SetVoltage(short alpha, short beta)
{
	int x, y, z, tmp;
	unsigned short cmp_a, cmp_b, cmp_c;

	/*basic voltage vector calcu*/
	tmp = alpha * sqrt3DIV_2;
	alpha = (short)(tmp >> 15); /*= ((3^0.5)/2)¦Á*/
	tmp = beta >> 1; /* = (1/2)¦Â*/

	x = beta;
	y = alpha - tmp; /*((3^0.5)¦Á-¦Â)/2*/
	z = - alpha - tmp; /*(-(3^0.5)¦Á-¦Â)/2*/

	/*sector calcu*/
	tmp =  (x > 0)?1:0;
	tmp += (y > 0)?2:0;
	tmp += (z > 0)?4:0;
	sector = tmp;

	/*basic time vector calcu*/
	tmp = vp;
	x *= T;
	x /= tmp; /*¦Â*/
	y *= T;
	y /= tmp; /*((3^0.5)¦Á-¦Â)/2*/
	z *= T;
	z /= tmp; /*(-(3^0.5)¦Á-¦Â)/2*/

	/*u-v-w duty cycles computation */
	switch(sector){
	case SECTOR_1:
		/*T1 = - y; (-(3^0.5)¦Á+¦Â)/2*/
		/*T2 = - z; (+(3^0.5)¦Á+¦Â)/2*/
		/*T0 = T - T1 - T2*/

		cmp_b = (unsigned short)((T + y + z) >> 1); /*= T0/2*/
		cmp_a = (unsigned short)(cmp_b - y); /*+ T1*/
		cmp_c = (unsigned short)(cmp_a - z); /*+ T2*/
		tmp = (cmp_a + cmp_c) >> 1;

		ADC1->JSQR = VSM_PHV << 15;
		ADC2->JSQR = VSM_PHW << 15;
		break;
	case SECTOR_2:
		/*T1 = - z; (+(3^0.5)¦Á+¦Â)/2*/
		/*T2 = - x; -¦Â*/
		/*T0 = T - T1 - T2*/

		cmp_a = (unsigned short)((T + z + x) >> 1); /*T0/2*/
		cmp_c = (unsigned short)(cmp_a - z); /*+ T1*/
		cmp_b = (unsigned short)(cmp_c - x); /*+ T2*/
		tmp = (cmp_c + cmp_b) >> 1;

		ADC1->JSQR = VSM_PHU << 15;
		ADC2->JSQR = VSM_PHW << 15;
		break;

	case SECTOR_3:
		/*T1 = y; (+(3^0.5)¦Á-¦Â)/2*/
		/*T2 = x; ¦Â*/
		/*T0 = T - T1 - T2*/

		cmp_a = (unsigned short)((T - y - x) >> 1); /*T0/2*/
		cmp_b = (unsigned short)(cmp_a + y); /*+ T1*/
		cmp_c = (unsigned short)(cmp_b + x); /*+ T2*/
		tmp = (cmp_b + cmp_c) >> 1;

		ADC1->JSQR = VSM_PHU << 15;
		ADC2->JSQR = VSM_PHW << 15;
		break;

	 case SECTOR_4:
		/*T1 = - x; -¦Â*/
		/*T2 = - y; (-(3^0.5)¦Á+¦Â)/2*/
		/*T0 = T - T1 - T2*/

		cmp_c = (unsigned short)((T + x + y) >> 1); /*T0/2*/
		cmp_b = (unsigned short)(cmp_c - x); /*+ T1*/
		cmp_a = (unsigned short)(cmp_b - y); /*+ T2*/
		tmp = (cmp_b + cmp_a) >> 1;

		ADC1->JSQR = VSM_PHU << 15;
		ADC2->JSQR = VSM_PHV << 15;
		break;

	case SECTOR_5:
		/*T1 = x; ¦Â*/
		/*T2 = z; (-(3^0.5)¦Á-¦Â)/2*/
		/*T0 = T - T1 - T2*/

		cmp_b = (unsigned short)((T - x -z) >> 1); /*T0/2*/
		cmp_c = (unsigned short)(cmp_b + x); /*+ T1*/
		cmp_a = (unsigned short)(cmp_c + z); /*+ T2*/
		tmp = (cmp_c + cmp_a) >> 1;

		ADC1->JSQR = VSM_PHU << 15;
		ADC2->JSQR = VSM_PHV << 15;
		break;

	case SECTOR_6:
		/*T1 = z; (-(3^0.5)¦Á-¦Â)/2*/
		/*T2 = y; (+(3^0.5)¦Á-¦Â)/2*/
		/*T0 = T - T1 - T2*/

		cmp_c = (unsigned short)((T - z - y) >> 1); /*T0/2*/
		cmp_a = (unsigned short)(cmp_c + z); /*+ T1*/
		cmp_b = (unsigned short)(cmp_a + y); /*+ T2*/
		tmp = (cmp_a + cmp_b) >> 1;

		ADC1->JSQR = VSM_PHV << 15;
		ADC2->JSQR = VSM_PHW << 15;
		break;
	default:
		break;
	}

	/* Load compare registers values */
	TIM1->CCR1 = cmp_a;
	TIM1->CCR2 = cmp_b;
	TIM1->CCR3 = cmp_c;

	/*trig the adc*/
	TIM1->CCR4 = (unsigned short)tmp;
}

#define INJ_ADC_BITS (15)
#define MA_PER_CNT (VSM_VREF_MV/VSM_RS_OHM/(1 << INJ_ADC_BITS))
#define CNT_2_MA(cnt) ((cnt * (int)(MA_PER_CNT*(1 << 20))) >> 20)

void vsm_GetCurrent(short *a, short *b)
{
	int i1, i2;

	i1 = ADC1->JDR1;
	i2 = ADC2->JDR1;

	/*convet current unit from count to mA*/
	i1 = CNT_2_MA(i1);
	i2 = CNT_2_MA(i2);

	/*normalize*/
	i1 = NOR_AMP(i1);
	i2 = NOR_AMP(i2);

	switch (sector){
	case 4:
	case 5: // measure a/b
		*a = (short) i1;
		*b = (short) i2;
		break;

	case 6:
	case 1: //measure b/c
		*a = (short) (-i1 - i2);
		*b = (short) i2;
		break;

	case 2:
	case 3: // measure a/c
		*a = (short) i1;
		*b = (short) (-i1 - i2);
		break;

	default:
		break;
	}
}

void vsm_Start(void)
{
	ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
	ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
	vsm_SetVoltage(0,0);
}

void vsm_Stop(void)
{
	ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
	TIM_Cmd(TIM1, DISABLE);
}

