/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
*/

#include "stm32f10x.h"
#include "ad9833.h"
#include "spi.h"
#include "vvt/vvt_pulse.h"

//pravite varibles define
static ad9833_t knock_dds;
static ad9833_t vss_dds;
static ad9833_t wss_dds;
static ad9833_t rpm_dds;
static int knock_dds_write_reg(int reg, int val);
static int vss_dds_write_reg(int reg, int val);
static int wss_dds_write_reg(int reg, int val);
static int rpm_dds_write_reg(int reg, int val);

static void vvt_pulse_adc_Init(void);
static void vvt_pulse_dds_Init(void);
static void vvt_pulse_gpio_Init(void);

static unsigned short adc_knock_frq;
static unsigned short adc_knock_frq_save;

//global varibles
unsigned short vvt_adc3[5];

void pss_Enable(int on)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = (on > 0) ? ENABLE : DISABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void vvt_pulse_Init(void)
{
	vvt_pulse_adc_Init();
	vvt_pulse_dds_Init();
	vvt_pulse_gpio_Init();
}

void knock_Update(void)
{
	int temp;

	adc_knock_frq = vvt_adc3[3];
	adc_knock_frq &= 0x0fff;
	if ((adc_knock_frq>>2) != (adc_knock_frq_save>>2)) {
		//status convert
		adc_knock_frq_save = adc_knock_frq;

		//calculate the frequency,fre:1K-20k = (1K + (0K-19K)),pot:0-10k
		temp = (19000 * adc_knock_frq_save)>>12;
		temp += 1000;

		knock_SetFreq((short)temp);	
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
	BitAction val = Bit_RESET;
		
	if(mv > 0)
		val = Bit_SET;

	pin = 1 << (2 + ch);
	GPIO_WriteBit(GPIOG, pin, val);
}

/*************** private members ********************/
//dds related define
static int knock_dds_write_reg(int reg, int val)
{ 
	int ret; 
	GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET); 
	ret = spi_Write(1, val); 
	GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET);
	return ret;
}

static int vss_dds_write_reg(int reg, int val)
{ 
	int ret; 
	GPIO_WriteBit(GPIOF, GPIO_Pin_11, Bit_RESET); 
	ret = spi_Write(1, val); 
	GPIO_WriteBit(GPIOF, GPIO_Pin_11, Bit_SET);
	return ret;
}

static int wss_dds_write_reg(int reg, int val)
{ 
	int ret; 
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET); 
	ret = spi_Write(1, val); 
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
	return ret;
}

int rpm_dds_write_reg(int reg, int val)
{
	int ret;
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_RESET);
	ret = spi_Write(1, val);
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET);
	return ret;
}

static ad9833_t knock_dds = {
	.io = {
		.write_reg = knock_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SIN,
};

static ad9833_t vss_dds = {
	.io = {
		.write_reg = vss_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t wss_dds = {
	.io = {
		.write_reg = wss_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

static ad9833_t rpm_dds = {
	.io = {
		.write_reg = rpm_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SQU | AD9833_OPT_DIV,
};

//for adc init
static void vvt_pulse_adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	/*quency adc input*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	
	/* Configure PF.09 (ADC Channel 6-10) as analog input*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 |\
								  GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	/* DMA2 channel5 configuration */
	DMA_DeInit(DMA2_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC3->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)vvt_adc3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 5;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA2_Channel5, &DMA_InitStructure);
	/* Enable DMA1 channel1 */
	DMA_Cmd(DMA2_Channel5, ENABLE);
  
	ADC_DeInit(ADC3);
	ADC_StructInit(&ADC_InitStructure);

	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 5;
	ADC_Init(ADC3, &ADC_InitStructure);

	/* ADC3 regular channel_6,7,8,9,10 configuration */
	ADC_RegularChannelConfig(ADC3, CH_NE58X, 1, ADC_SampleTime_28Cycles5);
	ADC_RegularChannelConfig(ADC3, CH_VSS, 2, ADC_SampleTime_28Cycles5);
	ADC_RegularChannelConfig(ADC3, CH_WSS, 3, ADC_SampleTime_28Cycles5);
	ADC_RegularChannelConfig(ADC3, CH_KNOCK_FRQ, 4, ADC_SampleTime_28Cycles5);
	ADC_RegularChannelConfig(ADC3, CH_MISFIRE_STREN, 6, ADC_SampleTime_28Cycles5);
	
	/* Enable ADC3 DMA */
	ADC_DMACmd(ADC3, ENABLE);
	/* Enable ADC1 */
	ADC_Cmd(ADC3, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC3);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC3));
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC3);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC3));

	/*start sample*/
	ADC_SoftwareStartConvCmd(ADC3, ENABLE);
}

static void vvt_pulse_dds_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/*knock chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET); //cs_dds_knock = 1

	/*VSS chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOF, GPIO_Pin_11, Bit_SET); //cs_dds_vss = 1

	/*WSS chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET); //cs_dds_wss = 1

	/*RPM select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOC, GPIO_Pin_5, Bit_SET); // cs_dds_rpm = 1

	/*chip init*/
	ad9833_Init(&knock_dds);
	ad9833_Init(&vss_dds);
	ad9833_Init(&wss_dds);
	ad9833_Init(&rpm_dds);
}

static void vvt_pulse_gpio_Init(void)
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);

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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4| \
								  GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/*rpm irq init*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
	EXTI_InitStruct.EXTI_Line = EXTI_Line0;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	pss_Enable(0);/*misfire input switch*/
}
