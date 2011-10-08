/*
*	miaofng@2010 initial version
		this routine is used to provide the device driver for vvt knock control
*/

#include "stm32f10x.h"
#include "ad9833.h"
#include "md204l.h"
#include "mcp23017.h"
#include "vvt_pulse.h"
#include <string.h>
#include "../misfire.h"
#include <stdio.h>

#define KNOCK_EN	GPIO_Pin_11
#define KNOCK_EN_PORT	GPIOB

#define WSS_MAX_RPM	20000
#define VSS_MAX_RPM	10000
#define KNOCK_MAX_FRQ	20000	//20k
#define NE58X_MAX_RPM	5000
#define MISFIRE_MAX_STRENGTH	0x8000;

static short wss_adc_value;
static short vss_adc_value;
static short knock_adc_value;
static short ne58x_adc_value;
static short misfire_adc_value;
static char misfire_pattern;

//global adc varibles
static short vvt_adc_buf[5];
short vvt_adc[5];

short md204l_read_buf[MD204L_READ_LEN];
short md204l_write_buf[MD204L_WRITE_LEN];

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

//md204l display related define
#define MD204L_UPDATE_PERIOD 500
static md204l_t hid_md204l = {
	.bus = &uart2,
	.station = 0x01,
};
static time_t md204l_update_timer;

//mcp23017 input related define
#define KNOCK_CONFIG_ADDR		0x13
#define MISFIRE_CONFIG_ADDR	0x12
static mcp23017_t vvt_mcp23017 = {
	.bus = &i2c1,
	.chip_addr = 0x40,
	.port_type = MCP23017_PORT_IN
};

static void vvt_adc_Init(void);
static void vvt_pulse_dds_Init(void);
static void vvt_pulse_gpio_Init(void);


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
	
	md204l_Init(&hid_md204l);
	md204l_update_timer  = time_get(MD204L_UPDATE_PERIOD);
	mdelay(100);	//delay 100ms for mcp23017 reset
	mcp23017_Init(&vvt_mcp23017);
	vvt_adc_Init();
	vvt_pulse_dds_Init();
	vvt_pulse_gpio_Init();
}

void vvt_pulse_Update(void)
{
	vvt_adc_Update();
	unsigned temp;
	short sub;

	//wss adc input changed
	sub = wss_adc_value - vvt_adc[4];
	if (sub > 0x20 || sub < -0x20) {
		wss_adc_value = vvt_adc[4];
		temp = wss_adc_value * WSS_MAX_RPM;
		temp >>= 12;
		wss_SetFreq((short)temp);
	}

	//vss adc input changed
	sub = vss_adc_value - vvt_adc[3];
	if (sub > 0x20 || sub < -0x20) {
		vss_adc_value = vvt_adc[3];
		temp = vss_adc_value * VSS_MAX_RPM;
		temp >>= 12;
		vss_SetFreq((short)temp);
	}

	//misfire strength adc input changed
	sub = misfire_adc_value - vvt_adc[2];
	if (sub > 0x20 || sub < -0x20) {
		misfire_adc_value = vvt_adc[2];
		temp = misfire_adc_value;
		temp <<= 15; //(32768 -> 100%)
		temp >>= 12;
		misfire_ConfigStrength((short)temp);
	}

	//misfire pattern update
	temp = misfire_GetPattern();
	if (misfire_pattern != temp) {
		misfire_pattern = temp;
		misfire_ConfigPattern((short)temp);
	}

	//knock freq adc input changed
	sub = knock_adc_value - vvt_adc[1];
	if (sub > 0x20 || sub < -0x20) {
		knock_adc_value = vvt_adc[1];
		temp = knock_adc_value * KNOCK_MAX_FRQ;
		temp >>= 12;
		knock_SetFreq((short)temp);
	}

	//ne58x adc input changed
	sub = ne58x_adc_value - vvt_adc[0];
	if (sub > 0x30 || sub < -0x30) {
		ne58x_adc_value = vvt_adc[0];
		temp = ne58x_adc_value * NE58X_MAX_RPM;
		temp >>= 12;
		md204l_write_buf[0] = temp;
		temp = temp?temp:1;
		misfire_SetSpeed(temp);
	}

	//communitcate with md204l
	if(time_left(md204l_update_timer) < 0) {
		md204l_update_timer  = time_get(MD204L_UPDATE_PERIOD);
		md204l_Read(&hid_md204l, MD204L_READ_ADDR, md204l_read_buf, MD204L_READ_LEN);
		md204l_Write(&hid_md204l, MD204L_WRITE_ADDR, md204l_write_buf, MD204L_WRITE_LEN);
		//printf("%d, %d, %d, %d, %d \n", md204l_write_buf[0], md204l_read_buf[0], md204l_read_buf[1], md204l_read_buf[2], md204l_read_buf[3]);
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

/*control the 74lvc1g66,analog switch*/
void knock_Enable(int en)
{
	if(en)
		GPIO_SetBits(KNOCK_EN_PORT, KNOCK_EN);
	else
		GPIO_ResetBits(KNOCK_EN_PORT, KNOCK_EN);
}

int knock_GetPattern(void)
{
#if 0
	unsigned char temp;
	mcp23017_ReadByte(&vvt_mcp23017, KNOCK_CONFIG_ADDR, 1, &temp);
	return temp & 0x3f;
#endif
	return 0x01;
}

int misfire_GetPattern(void)
{
#if 0
	unsigned char temp;
	mcp23017_ReadByte(&vvt_mcp23017, MISFIRE_CONFIG_ADDR, 1, &temp);
	return temp & 0x3f;
#endif
	return 0x01;
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
void vvt_adc_Update(void)
{
	for (int i = 0; i < 5; i++)
		vvt_adc[i] = vvt_adc_buf[i]&0x0fff;
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
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
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

	ADC_RegularChannelConfig(ADC1, CH_NE58X, 1, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_KNOCK_FRQ, 2, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_MISFIRE_STREN, 3, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_VSS, 4, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, CH_WSS, 5, ADC_SampleTime_239Cycles5);
	
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

	mdelay(1000);
	ad9833_Init(&knock_dds);
	ad9833_Init(&vss_dds);
	ad9833_Init(&wss_dds);
	ad9833_Init(&rpm_dds);
}

static void vvt_pulse_gpio_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct;

	/*knock enable*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = KNOCK_EN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(KNOCK_EN_PORT,  &GPIO_InitStructure);

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

