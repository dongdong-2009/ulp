/*
*  o2 sensor pulse tester
*  initial version
*
*/

#include "ulp/sys.h"
#include "nvm.h"
#include "led.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"
#include "ulp_time.h"
#include "stm32f10x.h"
#include "pwm.h"

#define vdda 3.300f //unit: V
#define vref 3.000f //unit: V
#define rifb 50.00f //unit: Ohm
#define v0 0.050f
#define v1 0.072f
#define i0 0.130f
#define i1 0.404f

/*if >0: o2pt = pulse mode*/
static int o2pt_pulse_ms[2];
static time_t o2pt_pulse_timer[2];

/*resources list
PC6		OCP_A
PC7		OCP_B

PC8		EPUL_A
PC9		IPOL_A
PC10	E500_A
PC12	EPUL_B
PC13	IPOL_B
PC14	E500_B

PD0		KEY0
PD1		KEY1
PD2		KEY2
PD3		KEY3

PD8		RLY0
PD9		RLY1
PD10	RLY2
PD11	RLY3
*/
void o2pt_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin =  \
		GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | \
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void o2pt_adc_init(void)
{
	/*
	PC0		VFB_A		ADC123_IN10
	PC1		VFB_B		ADC123_IN11
	PC2		IFB_A		ADC123_IN12
	PC3		IFB_B		ADC123_IN13
	PC4		VO2S_A		ADC12_IN14
	PC5		VO2S_B		ADC12_IN15
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = \
		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
		GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);
	ADC_DeInit(ADC2);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 4); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_13, 4, ADC_SampleTime_239Cycles5);
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_InjectedSequencerLengthConfig(ADC2, 2); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_14, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC2, ADC_Channel_15, 2, ADC_SampleTime_239Cycles5);
	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC2, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	ADC_Cmd(ADC2, ENABLE);
	ADC_ResetCalibration(ADC2);
	while(ADC_GetResetCalibrationStatus(ADC2));
	ADC_StartCalibration(ADC2);
	while(ADC_GetCalibrationStatus(ADC2)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC2, ENABLE);
}

void o2pt_dac_init(void)
{
	/*
	PA4		IPUL_A		DAC_OUT1
	PA5		IPUL_B		DAC_OUT2
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	/*to avoid parasitic consumption*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	DAC_InitTypeDef DAC_InitStructure;
	DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
	DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	DAC_Init(DAC_Channel_1, &DAC_InitStructure);
	DAC_Init(DAC_Channel_2, &DAC_InitStructure);

	DAC_Cmd(DAC_Channel_1, ENABLE);
	DAC_Cmd(DAC_Channel_2, ENABLE);

	/*
	PB6		VSET_A		TIM4_CH1
	PB7		VSET_B		TIM4_CH2
	PB8		ISET_A		TIM4_CH3
	PB9		ISET_B		TIM4_CH4
	*/
	const pwm_cfg_t cfg = {.hz = 100000, .fs = 1023};
	pwm41.init(&cfg);
	pwm42.init(&cfg);
	pwm43.init(&cfg);
	pwm44.init(&cfg);
}

void buck_set(int ch, float v, float i)
{
	/*according to the experiment:
	vfb = vset = 0.050 + v * 0.072
	ifb = iset = 0.130 + i * 0.404
	*/

	float vset, iset, dset;
	int regv, regi;

	vset = v * v1 + v0;
	dset = vset * 1024 / vdda;
	regv = (int) dset;

	iset = i * i1 + i0;
	dset = iset * 1024 / vdda;
	regi = (int) dset;

	/*
	PB6		VSET_A		TIM4_CH1
	PB7		VSET_B		TIM4_CH2
	PB8		ISET_A		TIM4_CH3
	PB9		ISET_B		TIM4_CH4
	*/

	if(ch == 0) {
		pwm41.set(regv);
		pwm43.set(regi);
	}
	else {
		pwm42.set(regv);
		pwm44.set(regi);
	}
}

float buck_vget(int ch)
{
	float vfb, vout;
	int regv;

	/*
	PC0		VFB_A		ADC123_IN10
	PC1		VFB_B		ADC123_IN11
	*/
	regv = (ch == 0) ? ADC1->JDR1 : ADC1->JDR2;
	vfb = vref * regv / 4096;

	/*according to the experiment:
	vfb = vset = 0.050 + v * 0.072
	=> v = (vfb - 0.050) / 0.072
	*/
	vout = (vfb - v0) / v1;
	return vout;
}

float buck_iget(int ch)
{
	float ifb, iout;
	int regi;

	/*
	PC2		IFB_A		ADC123_IN12
	PC3		IFB_B		ADC123_IN13
	*/
	regi = (ch == 0) ? ADC1->JDR3 : ADC1->JDR4;
	ifb = vref * regi / 4096;

	/*according to the experiment:
	ifb = iset = 0.130 + i * 0.404
	=> i = (ifb - 0.130) / 0.404
	*/
	iout = (ifb - i0) / i1;
	return iout;
}

float o2pt_vget(int ch, int e500)
{
	float vfb, vo2;
	int regv;

	/* change to measure mode
	PC8		EPUL_A
	PC9		IPOL_A
	PC10	E500_A
	PC12	EPUL_B
	PC13	IPOL_B
	PC14	E500_B
	*/
	if(ch == 0) {
		o2pt_pulse_ms[0] = 0;
		GPIOC->BRR = GPIO_Pin_8; //EPUL = 0
		if(e500) GPIOC->BSRR = GPIO_Pin_10; //E500 = e500
		else GPIOC->BRR = GPIO_Pin_10;
	}
	else {
		o2pt_pulse_ms[1] = 0;
		GPIOC->BRR = GPIO_Pin_12; //EPUL = 0
		if(e500) GPIOC->BSRR = GPIO_Pin_14; //E500 = e500
		else GPIOC->BRR = GPIO_Pin_14;
	}

	/*
	PC4		VO2S_A		ADC12_IN14
	PC5		VO2S_B		ADC12_IN15
	*/
	regv = (ch == 0) ? ADC2->JDR1 : ADC2->JDR2;
	vfb = vref * regv / 4096;

	/*amplified by opa330, gain = 3v/v*/
	vo2 = vfb / 3;
	return vo2;
}

void o2pt_pulse(int ch, float i, float hz)
{
	/*
	PA4		IPUL_A		DAC_OUT1
	PA5		IPUL_B		DAC_OUT2
	*/
	float vset, dset;
	int regv;

	/*iset = vset / rifb = > vset = iset * rifb */
	vset = i * rifb; //30mA * 50ohm = 1500mv
	dset = vset * 4096 / vref;
	regv = (int) dset;

	/*limit*/
	regv = (regv > 4095) ? 4095 : regv;
	regv = (regv < 0)? 0 : regv;

	/*change current & o2pt mode
	PC8		EPUL_A
	PC12	EPUL_B
	*/
	if(ch == 0) {
		DAC->DHR12R1 = regv;
		GPIOC->BSRR = GPIO_Pin_8; //EPUL = 1
	}
	else {
		DAC->DHR12R2 = regv;
		GPIOC->BSRR = GPIO_Pin_12; //EPUL = 1
	}

	//avoid switch too fast
	hz = (hz > 50.0f) ? 50.0f : hz;
	ch = (ch > 0) ? 1 : 0;

	o2pt_pulse_ms[ch] = (int) (1000.0f / hz);
	o2pt_pulse_timer[ch] = time_get(o2pt_pulse_ms[ch]);
}

void o2pt_init(void)
{
	o2pt_pulse_ms[0] = 0;
	o2pt_pulse_ms[1] = 0;
	o2pt_gpio_init();
	o2pt_adc_init();
	o2pt_dac_init();

	buck_set(0, 0, 0);
	buck_set(1, 0, 0);
	o2pt_vget(0, 0);
	o2pt_vget(1, 0);
}

void o2pt_update(void)
{
	int regv = 0;
	if(o2pt_pulse_ms[0]) {
		if(time_left(o2pt_pulse_timer[0]) < 0) {
			o2pt_pulse_timer[0] = time_get(o2pt_pulse_ms[0]);
			/* change pulse polar
			PC9		IPOL_A
			*/
			regv = GPIOC->ODR;
			regv &= GPIO_Pin_9;
			if(regv) GPIOC->BRR = GPIO_Pin_9;
			else GPIOC->BSRR = GPIO_Pin_9;
		}
	}

	if(o2pt_pulse_ms[1]) {
		if(time_left(o2pt_pulse_timer[1]) < 0) {
			o2pt_pulse_timer[1] = time_get(o2pt_pulse_ms[1]);
			/* change pulse polar
			PC13	IPOL_B
			*/
			regv = GPIOC->ODR;
			regv &= GPIO_Pin_13;
			if(regv) GPIOC->BRR = GPIO_Pin_13;
			else GPIOC->BSRR = GPIO_Pin_13;
		}
	}
}

int main(void)
{
	sys_init();
	o2pt_init();
	led_flash(LED_GREEN);
	printf("o2pt v1.0, build: %s %s\n\r", __DATE__, __TIME__);

	while(1) {
		sys_update();
		o2pt_update();
	}
}

#include "shell/cmd.h"
int cmd_buck_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"buck ch v i	to set buck voltage & over current\n"
		"buck ch v/i	to readback v or i\n"
		"buck ch		to readback v and i\n"
	};

	int ch;
	float v, i;

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	ch = atoi(argv[1]);
	if(argc == 4) {
		v = atof(argv[2]);
		i = atof(argv[3]);
		buck_set(ch, v, i);
		return 0;
	}

	if(argc == 3) {
		char c = argv[2][0];
		if(c == 'v') printf("<%+.3f\n\r", buck_vget(ch));
		else if(c == 'i') printf("<%+.3f\n\r", buck_iget(ch));
		else printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		v = buck_vget(ch);
		i = buck_iget(ch);
		printf("<%+.3f,%+.3f\n\r", v, i);
		return 0;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_buck = {"buck", cmd_buck_func, "buck power output ctrl"};
DECLARE_SHELL_CMD(cmd_buck)

int cmd_o2pt_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"vopen ch 		to measure o2s opened voltage\n"
		"vload ch		to measure o2s loaded voltage\n"
		"pulse ch i hz	to apply pulsed current on o2s\n"
	};

	int ch;
	float i, hz;

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	ch = atoi(argv[1]);
	if(argc == 4) { //apply pulsed current
		i = atof(argv[2]);
		hz = atof(argv[3]);
		o2pt_pulse(ch, i, hz);
		return 0;
	}

	if(argc == 2) {
		if(!strcmp(argv[0], "vopen")) printf("<%+.3f\n\r", o2pt_vget(ch, 0));
		else if(!strcmp(argv[0], "vload")) printf("<%+.3f\n\r", o2pt_vget(ch, 1));
		else printf("%s", usage);
		return 0;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_vopen = {"vopen", cmd_o2pt_func, "o2pt opened voltage measure"};
DECLARE_SHELL_CMD(cmd_vopen)
const cmd_t cmd_vload = {"vload", cmd_o2pt_func, "o2pt loaded voltage measure"};
DECLARE_SHELL_CMD(cmd_vload)
const cmd_t cmd_pulse = {"pulse", cmd_o2pt_func, "o2pt apply pulsed current"};
DECLARE_SHELL_CMD(cmd_pulse)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?			to read identification string\n"
		"*RST			instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<ulikar technoledge,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		NVIC_SystemReset();
	}
	else {
		printf("%s", usage);
		return 0;
	}
	return 0;
}
