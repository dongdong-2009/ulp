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
#define SW1_FBC		0x01
#define SW2_FBC		0x02
#define SW3_FBC		0x04
#define SW4_FBC		0x08
#define SW5_FBC		0x10

#define LOOP_FBC	0x20

#define LED1_FBC	0x01
#define LED2_FBC	0x02
#define LED3_FBC	0x04
#define LED4_FBC	0x08
#define LED5_FBC	0x10

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

	//shut down all ad channel
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, 0x00);
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, 0x00);

	//adc init
	c131_adc_Init();
}

int c131_DiagSW(void)
{
	int temp;
	int result = 0;

	//DIAG FOR SW1
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, SW1_FBC);
	sw_SetRelayStatus(C131_SW1_C1, RELAY_OFF);
	sw_SetRelayStatus(C131_SW1_C2, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("14MA is %d \n", temp);
#endif
	if ((temp <= SW1_14MA_LOW_LIMIT) || (temp >= SW1_14MA_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	sw_SetRelayStatus(C131_SW1_C2, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("6MA is %d \n", temp);
#endif
	if ((temp <= SW1_6MA_LOW_LIMIT) || (temp >= SW1_6MA_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	//DIAG FOR SW2 PAD
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, SW2_FBC);
	sw_SetRelayStatus(C131_SW2_C1, RELAY_OFF);
	sw_SetRelayStatus(C131_SW2_C2, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("2K82 is %d \n", temp);
#endif
	if ((temp <= SW2_2K82_LOW_LIMIT) || (temp >= SW2_2K82_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	sw_SetRelayStatus(C131_SW2_C2, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("6MA is %d \n", temp);
#endif
	if ((temp <= SW2_820_LOW_LIMIT) || (temp >= SW2_820_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	//DIAG FOR SW3
	//open AD channel for sw3(DSB)
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, SW3_FBC);
	sw_SetRelayStatus(C131_SW3, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("DSB is %d \n", temp);
#endif
	temp >>= 1;
	if (temp <= 0) {
		result = ERROR_SWITCH;
	}
	sw_SetRelayStatus(C131_SW3, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("DSB is %d \n", temp);
#endif
	temp >>= 1;
	if (temp >= 0) {
		result = ERROR_SWITCH;
	}

	//DIAG FOR SW4
	//open AD channel for sw4(PSB)
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, SW4_FBC);
	sw_SetRelayStatus(C131_SW4, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("DSB is %d \n", temp);
#endif
	temp >>= 1;
	if (temp <= 0) {
		result = ERROR_SWITCH;
	}
	sw_SetRelayStatus(C131_SW4, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("DSB is %d \n", temp);
#endif
	temp >>= 1;
	if (temp >= 0) {
		result = ERROR_SWITCH;
	}

	//DIAG FOR SW5 SBR
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, SW5_FBC);
	sw_SetRelayStatus(C131_SW5_C1, RELAY_OFF);
	sw_SetRelayStatus(C131_SW5_C2, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("900 is %d \n", temp);
#endif
	if ((temp <= SW5_900_LOW_LIMIT) || (temp >= SW5_900_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	sw_SetRelayStatus(C131_SW5_C2, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("220 is %d \n", temp);
#endif
	if ((temp <= SW5_220_LOW_LIMIT) || (temp >= SW5_220_HIGH_LIMIT)){
		result = ERROR_SWITCH;
	}

	//shut down all feedback channel
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, 0x00);

	return result;
}

int c131_DiagLOOP(void)
{
	int i;
	int temp;
	int result = 0;

	//open the loop feedback channel
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, LOOP_FBC);

	//shift the all loop relays to DIAG status
	loop_SetRelayStatus(0x0fff, RELAY_ON);
	c131_relay_Update();

	//relay set
	loop_SetRelayStatus(C131_LOOP, RELAY_ON);
	for (i = 0; i < 12; i++) {
		loop_SetRelayStatus(C131_LOOP1 << i, RELAY_OFF);
		c131_relay_Update();
		temp = c131_adc_GetValue();
		loop_SetRelayStatus(C131_LOOP1 << i, RELAY_ON);
		c131_relay_Update();
#if DIAG_DEBUG
		printf("loop %d value is %d \n", i+1, temp);
#endif
		if ((temp > LOOP_HIGH_LIMIT) || (temp < LOOP_LOW_LIMIT)) {
			result = ERROR_LOOP;
		}
	}

	//go to default status
	loop_SetRelayStatus(C131_LOOP | 0x0fff, RELAY_OFF);
	c131_relay_Update();

	//shut down all feedback channel
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOB, 0x00);
	return result;
}

int c131_DiagLED(void)
{
	int temp;
	int result = 0;

	//for led2 diagnosis
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, LED2_FBC);
	led_SetRelayStatus(C131_LED2, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED2 is %d \n", temp);
#endif
	temp >>= 2;
	if (temp > 0) {
		result = ERROR_LED;
	}

	led_SetRelayStatus(C131_LED2, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED2 is %d \n", temp);
#endif
	if ((temp > LED_HIGH_LIMIT) || (temp < LED_LOW_LIMIT)) {
		result = ERROR_LED;
	}

	//for led3 diagnosis
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, LED3_FBC);
	led_SetRelayStatus(C131_LED3, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED3 is %d \n", temp);
#endif
	temp >>= 2;
	if (temp > 0) {
		result = ERROR_LED;
	}

	led_SetRelayStatus(C131_LED3, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED3 is %d \n", temp);
#endif
	if ((temp > LED_HIGH_LIMIT) || (temp < LED_LOW_LIMIT)) {
		result = ERROR_LED;
	}

	//for led5 diagnosis
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, LED5_FBC);
	led_SetRelayStatus(C131_LED5, RELAY_ON);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED5 is %d \n", temp);
#endif
	temp >>= 2;
	if (temp > 0) {
		result = ERROR_LED;
	}

	led_SetRelayStatus(C131_LED5, RELAY_OFF);
	c131_relay_Update();
	temp = c131_adc_GetValue();
#if DIAG_DEBUG
	printf("LED5 is %d \n", temp);
#endif
	if ((temp > LED_HIGH_LIMIT) || (temp < LED_LOW_LIMIT)) {
		result = ERROR_LED;
	}

	//shut all adc channel
	mcp23s17_WriteByte(&mcp23s17, ADDR_GPIOA, 0x00);
	return result;
}

static void c131_adc_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	/*frequency adc input*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC_Channel_11, 1, ADC_SampleTime_239Cycles5);
	ADC_Cmd(ADC2, ENABLE);

	/* Enable ADC2 reset calibaration register */
	ADC_ResetCalibration(ADC2);
	/* Check the end of ADC2 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC2));
	/* Start ADC2 calibaration */
	ADC_StartCalibration(ADC2);
	/* Check the end of ADC2 calibration */
	while(ADC_GetCalibrationStatus(ADC2));
}

static int c131_adc_GetValue(void)
{
	int value;
	//start sample
	ADC_SoftwareStartConvCmd(ADC2, ENABLE);
	//wait till adc convert over
	while(!ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC));
	value = ADC_GetConversionValue(ADC2);
	value &= 0x0fff;
	return value;
}
