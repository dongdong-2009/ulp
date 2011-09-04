/*
 * David@2011,9 initial version
 */
#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "mcp23s17.h"
#include "c131_diag.h"
#include "c131_relay.h"

//ADC Port define
#define CH_DIAG_ADC		ADC_Channel_11
#define DIAG_PIN		GPIO_Pin_1
#define DIAG_PORT		GPIOC

//define multi ADC channel control port
#define SW1_FBC		0x00
#define SW2_FBC		0x01
#define SW3_FBC		0x02
#define SW4_FBC		0x03
#define SW5_FBC		0x04

#define LOOP_FBC	0x05

#define LED1_FBC	0x00
#define LED2_FBC	0x01
#define LED3_FBC	0x02
#define LED4_FBC	0x03
#define LED5_FBC	0x04

//local function define
static void c131_adc_Init(void);
static int c131_adc_GetValue(void);

//pravite varibles define
static mcp23s17_t mcp23s17 = {
	.bus = &spi2,
	.idx = SPI_CS_PD12,
	.option = MCP23S17_LOW_SPEED | MCP23017_PORTA_OUT | MCP23017_PORTB_OUT,
};

void c131_diag_Init(void)
{
	//spi device init
	mcp23s17_Init(&mcp23s17);

	//adc init
	c131_adc_Init();
}

int c131_DiagSW(void)
{
	unsigned char data;
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, data);
	
	return 0;
}

int c131_DiagLOOP(void)
{
	unsigned char data;
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, data);
	
	return 0;
}

int c131_DiagLED(void)
{
	unsigned char data;
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, data);
	
	return 0;
}

static void c131_adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	/*frequency adc input*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_239Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC1 reset calibaration register */
	ADC_ResetCalibration(ADC1);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1));
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));
}

static int c131_adc_GetValue(void)
{
	int value;
	//start sample
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	//wait till adc convert over
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	value = ADC_GetConversionValue(ADC1);
	value &= 0x0fff;
	return value
}
