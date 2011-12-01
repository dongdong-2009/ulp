/*
 *	sky@2011 initial version
 *	miaofng@2011 rewrite
 *	note:
 *		TIM3 is shared by pwm output(pin remap) & learning capture function!!!
 */
#include "config.h"
#include "ulp/debug.h"
#include "stm32f10x.h"

/*ori: ADDRESS_GPIO_Init*/
void card_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	//card slot, A0~A4 = PC1~5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* player
	SLAVE_PWM_HIGH		PC8/TIM3_CH3*
	SLAVE_PWM_LOW		PC9/TIM3_CH4*
	SLAVE_SEND		PA1/TIM2_CH2(HW BUG, PA0/TIM2_CH1 DMA CONFLICT WITH CNSL UART1)
	*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC, GPIO_InitStructure.GPIO_Pin);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#if 1 //pcb bug, PA0, PA1 connect together
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	/* recorder
	YIMA			PB11
	SLAVE_RECV		PB1/TIM3_CH4
	SLAVE_RECV		PB0/TIM3_CH3
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_InitStructure.GPIO_Pin);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* sensor power-up event detection
	ADC_SLAVE		PC0/ADC12_10
	*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	ADC_InitTypeDef ADC_InitStructure;
        ADC_DeInit(ADC1);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);
	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*ori: ADDRESS_GPIO_Read*/
int card_getslot(void)
{
	int adress=0;
	char scan_bit;
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
	adress|=(scan_bit<<0);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
	adress|=(scan_bit<<1);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
	adress|=(scan_bit<<2);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);
	adress|=(scan_bit<<3);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
	adress|=(scan_bit<<4);
	return (adress);
}

static char card_power_on = 0;
static char card_power_cnt = 0;
int card_getpower(void)
{
	if(ADC_GetFlagStatus (ADC1, ADC_FLAG_EOC)) {
		int mv = ADC_GetConversionValue(ADC1);
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		mv = mv * 3300 / 4096 * 440 / 110; /*110K/(110K + 330K)*/
		if(card_power_on == 1) {
			if(mv < 3000) {
				card_power_cnt --;
				if(card_power_cnt < 0) {
					card_power_on = 0;
					card_power_cnt = 0;
				}
			}
			else {
				card_power_cnt ++;
				if(card_power_cnt > 5) {
					card_power_cnt = 5;
				}
			}
		}
		else {
			if(mv > 7000) {
				card_power_cnt ++;
				if(card_power_cnt > 5) {
					card_power_on = 1;
					card_power_cnt = 5;
				}
			}
			else {
				card_power_cnt --;
				if(card_power_cnt < 0) {
					card_power_cnt = 0;
				}
			}
		}
	}

	return card_power_on;
}

/*
SLAVE_PWM_HIGH		PC8/TIM3_CH3*
SLAVE_PWM_LOW		PC9/TIM3_CH4*
SLAVE_SEND		PA1/TIM2_CH2(HW BUG, PA0/TIM2_CH1 DMA CONFLICT WITH CNSL UART1)
*/
/*ori: TIM3_init*/
/*ori: TIM3ch3_init*/
/*ori: TIM3ch4_init*/
int card_player_init(int min, int max, int div) //MIN/MAX CURRENT + SPEED
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	//close learn mode power supply (PB11)
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);

	//TIM2 -> main player(pulse generator)
	TIM_Cmd(TIM2, DISABLE);
	TIM_DeInit(TIM2);
	TIM_TimeBaseStructure.TIM_Period = 0xffff;
	TIM_TimeBaseStructure.TIM_Prescaler = div - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
	TIM_OCInitStructure.TIM_OutputState = TIM_ForcedAction_Active;
	TIM_OCInitStructure.TIM_Pulse = 0xffff;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);

	//TIM3 -> pwm, set low/high limit of current modulation
	TIM_DeInit(TIM3);
	TIM_TimeBaseStructure.TIM_Period = 1500;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 900;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_Pulse = 100;
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_Cmd(TIM3, ENABLE);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);
	return 0;
}

int card_player_start(unsigned short *fifo, int n, int repeat)
{
	if(repeat) {
		//time base re-init for repeat mode
		TIM2->ARR = fifo[n];
		fifo[n] = 1;
		n ++;
	}

	DMA_InitTypeDef DMA_InitStructure;
	DMA_Cmd(DMA1_Channel7, DISABLE);
	DMA_DeInit(DMA1_Channel7);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)( &(TIM2->CCR2));;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)(fifo);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = (repeat) ? DMA_Mode_Circular : DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel7, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel7, ENABLE);
	TIM_DMACmd(TIM2, TIM_DMA_CC2, ENABLE);

	TIM_SetCounter(TIM2, 0);
	TIM_SetCompare2(TIM2, 1); //first start bit always at time = 1

	TIM_SelectOCxM(TIM2, TIM_Channel_2, TIM_ForcedAction_Active); //idle
	TIM_CCxCmd(TIM2, TIM_Channel_2, TIM_CCx_Enable);
	//TIM_SelectOCxM(TIM2, TIM_Channel_2, TIM_ForcedAction_InActive); //idle
	//TIM_CCxCmd(TIM2, TIM_Channel_2, TIM_CCx_Enable);
	TIM_SelectOCxM(TIM2, TIM_Channel_2, TIM_OCMode_Toggle);
	TIM_CCxCmd(TIM2, TIM_Channel_2, TIM_CCx_Enable);
	TIM_Cmd(TIM2, ENABLE);
	return 0;
}

int card_player_left(void)
{
	int n = DMA_GetCurrDataCounter(DMA1_Channel7);
	if(n == 0) {
		//wait for last bit sent out
		while(!TIM_GetFlagStatus(TIM2, TIM_FLAG_CC2));
	}
	return n;
}

/*
	note: this routine should be always called by caller
	asap to pause the current modulation output, or timebase
	will roll over, and lead to output unncessary changes
*/
int card_player_stop(void)
{
	TIM_ClearFlag(TIM2, TIM_FLAG_CC2);
	TIM_Cmd(TIM2, DISABLE);
	TIM_SelectOCxM(TIM2, TIM_Channel_2, TIM_ForcedAction_Active); //idle
	TIM_CCxCmd(TIM2, TIM_Channel_2, TIM_CCx_Enable);
	return 0;
}

/*
YIMA			PB11
SLAVE_RECV		PB1/TIM3_CH4
SLAVE_RECV		PB0/TIM3_CH3
*/
/*ori: capture_sss_Init*/
/*ori: TIM_DMAC_Init*/
int card_recorder_init(void *cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	//halt current modulation output(Imax = Imin = 0)
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);

	//tim3 -> capture
	TIM_Cmd(TIM3, DISABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, DISABLE);

	TIM_TimeBaseStructure.TIM_Period = 0xffff;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; //1uS per count
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ICInitTypeDef  TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInit(TIM3, &TIM_ICInitStructure);

	//power up external sensor
	GPIO_SetBits(GPIOB, GPIO_Pin_11);
	return 0;
}

int card_recorder_start(void *fifo, int n, int repeat)
{
	DMA_InitTypeDef DMA_InitStructure;
	DMA_Cmd(DMA1_Channel3, DISABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)( &(TIM3->CCR4));;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)(fifo);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = n;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = (repeat) ? DMA_Mode_Circular : DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel3, ENABLE);

	TIM_DMACmd(TIM3, TIM_DMA_CC4, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	return 0;
}

int card_recorder_left(void)
{
	return DMA_GetCurrDataCounter(DMA1_Channel3);
}

int card_recorder_stop(void)
{
	//power down external sensor
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	TIM_Cmd(TIM3, DISABLE);
	return 0;
}
