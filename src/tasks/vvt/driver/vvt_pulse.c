/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
*/

#include "stm32f10x.h"
#include "ad9833.h"
#include "spi.h"
#include "vvt/vvt_pulse.h"
#include <string.h>

//pravite varibles define
static ad9833_t knock_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PC5,
	.option = AD9833_OPT_OUT_SIN,
};

static ad9833_t vss_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB0, //real one
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t wss_dds = {
	.bus = &spi1,
	.idx = SPI_CS_PB1,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t rpm_dds = {
	.bus = &spi2,
	.idx = SPI_CS_PB12,
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV | AD9833_OPT_SPI_DMA,
};

static void vvt_adc_Init(void);
static void vvt_pulse_dds_Init(void);
static void vvt_pulse_gpio_Init(void);

static unsigned short adc_knock_frq;
static unsigned short adc_knock_frq_save;

//global varibles
static unsigned short vvt_adc_buf[5];
unsigned short vvt_adc[5];

void pss_Enable(int on)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = (on > 0) ? ENABLE : DISABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void vvt_pulse_Init(void)
{
	vvt_adc_Init();
	vvt_pulse_dds_Init();
	vvt_pulse_gpio_Init();
}

void knock_Update(void)
{
	int temp;

	adc_knock_frq = vvt_adc[3];
	adc_knock_frq &= 0x0fff;
	if ((adc_knock_frq>>2) != (adc_knock_frq_save>>2)) {
		//status convert
		adc_knock_frq_save = adc_knock_frq;

		//calculate the frequency,fre:1K-20k = (1K + (0K-19K)),pot:0-10k
		temp = (19000 * adc_knock_frq_save)>>12;
		temp += 1000;

		//knock_SetFreq((short)temp);	
	}

}

void knock_SetFreq(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_KNOCK_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&knock_dds, fw);
}

void knock_SetVolt(knock_ch_t ch, short mv)
{
#if 0
	short pos;
	pos = (mv * 255) / CONFIG_DRIVER_KNOCK_MVPP_MAX;
	pos = (pos > 255) ? 255 : pos;
	mcp41x_SetPos(&knock_pot[ch], pos);
#endif
}

int knock_GetPattern(void)
{
	short temp;
	temp = GPIO_ReadInputData(GPIOF);
	temp &= GPIO_KNOCK_PATTERN;

	return (int)temp;
}

/*control the 74lvc1g66,analog switch*/
void knock_Enable(int en)
{
	if(en)
		GPIO_SetBits(GPIOG, GPIO_Pin_7);
	else
		GPIO_ResetBits(GPIOG, GPIO_Pin_7);
}

void pss_SetSpeed(short hz)
{
	unsigned mclk = (CONFIG_DRIVER_RPM_DDS_MCLK >> 16);
	unsigned fw = hz;
	fw <<= 16;
	fw /= mclk;
	ad9833_SetFreq(&rpm_dds, fw);
}

void pss_SetVolt(pss_ch_t ch, short mv)
{
	uint16_t pin;
	BitAction val = Bit_SET;

	if(mv > 0)
		val = Bit_RESET;

	if (ch == CAM4X_EXT) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_8, val);
	} else {
		pin = 1 << (7 + ch);
		GPIO_WriteBit(GPIOC, pin, val);
	}
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

//for adc update
void vvt_adc_update(void)
{
	for (int i = 0; i < 5; i++)
		vvt_adc[i] = vvt_adc_buf[i];
}

//for adc init
static void vvt_adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	/*quency adc input*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	
	/* Configure PF.09 (ADC Channel 6-10) as analog input*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 |\
								  GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	/* DMA2 channel5 configuration */
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&vvt_adc_buf[0];
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 5;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_PeripheralInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	/* Enable DMA1 channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);
  
	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 5;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, CH_NE58X, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_KNOCK_FRQ, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_MISFIRE_STREN, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_VSS, 1, ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_WSS, 1, ADC_SampleTime_55Cycles5);
	
	/* Enable ADC1 DMA */
	ADC_DMACmd(ADC1, ENABLE);
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1));
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));

	/*start sample*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static void vvt_pulse_dds_Init(void)
{
	//for rpm dds
	static char rpmdds_rbuf[6];
	static char rpmdds_wbuf[6];
	rpm_dds.p_rbuf = rpmdds_rbuf;
	rpm_dds.p_wbuf = rpmdds_wbuf;

	//ad9833_Init(&knock_dds);
	ad9833_Init(&vss_dds);
	ad9833_Init(&wss_dds);
	ad9833_Init(&rpm_dds);
}

static void vvt_pulse_gpio_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;


	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_KNOCK_PATTERN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/*knock input switch*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_KNOCK_PATTERN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	/*knock enable*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/*pulse output gpio pins*/
	//58X->PIN7, CAM1X->PIN8, CAM4X-IN->PIN9, CAM4X-EXT->PIN8
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

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

	pss_Enable(0);/*misfire input switch*/
}
