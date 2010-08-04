/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
*/

#include "stm32f10x.h"
#include "knock.h"
#include "ad9833.h"
#include "mcp41x.h"
#include "spi.h"

#define DECLARE_IO_FUNC(chip) \
	int chip##_write_reg(int reg, int val) \
	{ \
		int ret; \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_RESET); \
		ret = spi_Write(chip##_bus, val); \
		GPIO_WriteBit(chip##_band, chip##_pin, Bit_SET); \
		return ret; \
	}

/*pin map*/
#define knock_dds_bus	1
#define knock_dds_band	GPIOC //CS_DDS_KNOCK
#define knock_dds_pin	GPIO_Pin_4
DECLARE_IO_FUNC(knock_dds)

#if 0
#define knock_pot0_bus	2
#define knock_pot0_band	GPIOE //CS_VR_KS0
#define knock_pot0_pin	GPIO_Pin_11
DECLARE_IO_FUNC(knock_pot0)

#define knock_pot1_bus	2
#define knock_pot1_band	GPIOE //CS_VR_KS1
#define knock_pot1_pin	GPIO_Pin_12
DECLARE_IO_FUNC(knock_pot1)

#define knock_pot2_bus	2
#define knock_pot2_band	GPIOE //CS_VR_KS2
#define knock_pot2_pin	GPIO_Pin_13
DECLARE_IO_FUNC(knock_pot2)

#define knock_pot3_bus	2
#define knock_pot3_band	GPIOE //CS_VR_KS3
#define knock_pot3_pin	GPIO_Pin_14
DECLARE_IO_FUNC(knock_pot3)
#endif

static ad9833_t knock_dds = {
	.io = {
		.write_reg = knock_dds_write_reg,
		.read_reg = 0,
	},
	.option = AD9833_OPT_OUT_SIN,
};

#if 0
static mcp41x_t knock_pot[NR_OF_KS] = {
	[0] = {
		.io = {
			.write_reg = knock_pot0_write_reg,
			.read_reg = 0,
		},
	},

	[1] = {
		.io = {
			.write_reg = knock_pot1_write_reg,
			.read_reg = 0,
		},
	},

	[2] = {
		.io = {
			.write_reg = knock_pot2_write_reg,
			.read_reg = 0,
		},
	},

	[3] = {
		.io = {
			.write_reg = knock_pot3_write_reg,
			.read_reg = 0,
		},
	},
};
#endif

#define GPIO_KNOCK_PATTERN	(GPIO_Pin_All & 0x003f)
#define CH_NE58X			ADC_Channel_6
#define CH_VSS				ADC_Channel_7
#define CH_WSS				ADC_Channel_8
#define CH_KNOCK_FRQ		ADC_Channel_9
#define CH_MISFIRE_STREN	ADC_Channel_10
#define ADC3_DR_Address    ((uint32_t)0x40013C4C)

//pravite varibles define
static unsigned short adc_knock_frq;
static unsigned short adc_knock_frq_save;

//global varibles
unsigned short vvt_adc3[5];

void knock_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;	
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;	
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable DMA1 channel6 IRQ Channel */
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

	/* DMA1 channel1 configuration */
	DMA_DeInit(DMA2_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC3_DR_Address;
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
	/* Enable DMA1 Channel6 Transfer Complete interrupt */
	DMA_ITConfig(DMA2_Channel5, DMA_IT_TC, ENABLE);
	/* Enable DMA1 channel1 */
	DMA_Cmd(DMA2_Channel5, ENABLE);
  
	/*knock frequency adc input*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	
	/* Configure PF.09 (ADC Channel 6-10) as analog input*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 |\
								  GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

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

	/*chip select pins*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET); //cs_dds_knock = 1

#if 0
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_WriteBit(GPIOE, GPIO_Pin_11, Bit_SET); //cs_vr_ks0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_12, Bit_SET); //cs_vr_ks1 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_13, Bit_SET); //cs_vr_ks2 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_14, Bit_SET); //cs_vr_ks3 = 1
#endif

	/*chip init*/
	ad9833_Init(&knock_dds);
#if 0
	mcp41x_Init(&knock_pot[KS1]);
	mcp41x_Init(&knock_pot[KS2]);
	mcp41x_Init(&knock_pot[KS3]);
	mcp41x_Init(&knock_pot[KS4]);
#endif
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
