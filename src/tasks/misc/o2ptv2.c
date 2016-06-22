/*
*  o2 sensor pulse tester V2.0
*  initial version
*
*  resource usage:
*  PB10/SPI2	MATRIX/MBI_OE
*  PB12/SPI2	MATRIX/MBI_LE
*  PB13/SPI2	MATRIX/MBI_CK
*  PB15/SPI2	MATRIX/MBI_MOSI
*
*  PA4/DAC1 	BUCK/VSET
*  PC0/ADC10	BUCK/VGET
*  PC1/ADC11	BUCK/IGET
*  MBI_OUT10	BUCK/EBUCK
*
*  PB6/PWM		DMM/ISET
*  PC2/ADC12	DMM/VDMM
*  PD8/GPI		DMM/IOPEN
*  MBI_OUT11	DMM/EGA
*  MBI_OUT12	DMM/ERL
*  MBI_OUT13	DMM/EIS
*
*  MBI_OUT15	DIO/RLY0
*  MBI_OUT14	DIO/RLY1
*  PD0/GPI		DIO/KEY0
*  PD1/GPI		DIO/KEY1
*  PD2/GPI		DIO/KEY2
*  PD3/GPI		DIO/KEY3
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
#include "spi.h"
#include "mbi5025.h"

#define vref 3.000f //unit: V

/*if >0: o2pt = pulse mode*/
static int o2pt_pulse_ms;
static time_t o2pt_pulse_timer;
static int o2pt_pulse_pol = 0;

static const mbi5025_t mbi = {.bus = &spi2, .load_pin = SPI_2_NSS};
static unsigned short mbi_image = 0;

enum {
	EH = 0,
	BK,
	GY,
	W1,
	W2,

	EBUCK = 10,
	EGA,
	ERL,
	EIS,
	RLY1,
	RLY0,
};

/*
*  resource usage:
*  PB10/SPI2	MATRIX/MBI_OE
*  PB12/SPI2	MATRIX/MBI_LE
*  PB13/SPI2	MATRIX/MBI_CK
*  PB15/SPI2	MATRIX/MBI_MOSI
*/
void o2pt_rly_init(void)
{
	spi2.csel(SPI_CS_PB10, 1);
	mbi5025_Init(&mbi);
	mbi_image = 0;
	mbi5025_write_and_latch(&mbi, &mbi_image, sizeof(mbi_image));
	spi2.csel(SPI_CS_PB10, 0);
}

int o2pt_rly_get(int idx)
{
	int status = (mbi_image >> idx) & 0x01;
	return status;
}

void o2pt_rly_set(int idx, int on)
{
	mbi_image &= ~(1 << idx);
	if(on) {
		mbi_image |= 1 << idx;
	}

	mbi5025_write_and_latch(&mbi, &mbi_image, sizeof(mbi_image));
}

void o2pt_mxc_set(int apin, int bpin)
{
	mbi_image &= ~ 0x3ff;
	mbi_image |= 1 << (apin * 2);
	mbi_image |= 1 << (bpin * 2 + 1);
	mbi5025_write_and_latch(&mbi, &mbi_image, sizeof(mbi_image));
}

/*resources list
PD0		KEY0
PD1		KEY1
PD2		KEY2
PD3		KEY3
PD8		DMM/IOPEN
*/
void o2pt_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 | \
		GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void o2pt_adc_init(void)
{
	/*
	PC0		BUCK/VGET		ADC123_IN10
	PC1		BUCK/IGET		ADC123_IN11
	PC2		DMM/VDMM		ADC123_IN12
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = \
		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 3); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_239Cycles5);
	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void o2pt_dac_init(void)
{
	/*
	PA4		BUCK/VSET	DAC_OUT1
	PA5		N.A.
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
	*/
	const pwm_cfg_t cfg = {.hz = 100000, .fs = 1023};
	pwm41.init(&cfg);
}

void buck_set(float v, float i)
{
	/*according to the circuit:
	vbuck = vrxl(xl4016) + 10k * i
	= vrxl + 10k * v / 0.5k
	= vrxl + 10k * vset * 4.7k/(10k + 4.7k) / 0.5k
	= vrxl + vset * gain <gain = 10k * 4.7k/(10k + 4.7k) / 0.5k>
	=> vset = (vbuck - vrxl) / gain
	*/

	float vset, dset;
	int regv;

	float vrxl = 1.25; //xl4016
	float gain = 10.0 / 0.5 * 4.7/(4.7 + 10.0); // = 6.4

	vset = (v - vrxl) / gain;
	dset = vset * 4096 / vref;
	regv = (int) dset;

	/*limit*/
	regv = (regv > 4095) ? 4095 : regv;
	regv = (regv < 0)? 0 : regv;
	DAC->DHR12R1 = regv;
	o2pt_rly_set(EBUCK, v > 1.0);
}

float buck_vget()
{
	float vfb, vout;
	int regv;

	/*
	PC0		BUCK/VGET		ADC123_IN10
	*/
	regv = ADC1->JDR1;
	vfb = vref * regv / 4096;

	/*according to the circuit:
	vfb = vout * 3k / (27k + 3k)
	*/
	vout = vfb * 10;
	return vout;
}

float buck_iget()
{
	float ifb, iout;
	int regi;

	/*
	PC1		BUCK/IGET		ADC123_IN11
	*/
	regi = ADC1->JDR2;
	ifb = vref * regi / 4096;

	/*according to the circuit:
	ifb = iout * rsense * gina193
	=> iout = ifb / rsense / gina193
	*/
	float rsense = 0.020; //unit: ohm
	float gina193 = 20; //unit: v/v
	iout = ifb / (rsense * gina193);
	return iout;
}

float o2pt_res(int apin, int bpin, float iout, float gain)
{
	float df, gm;
	int regv;

	o2pt_rly_set(EBUCK, 0);
	o2pt_rly_set(ERL, 0);
	o2pt_rly_set(EIS, 1);
	o2pt_rly_set(EGA, gain >= 99.5);
	o2pt_mxc_set(apin, bpin);

	/* iout
	= vref * df * 4.7k / (4.7k + 22k) / 10 ohm
	= vref * df * gm
	=> df = iout / vref / gm
	*/
	gm = 4.7 / (4.7 + 22) / 10.0;
	df = iout / vref / gm;
	regv = (int)(df * 1023);

	/*limit*/
	regv = (regv > 1023) ? 1023 : regv;
	regv = (regv < 0)? 0 : regv;

	/*
	PB6		VSET_A		TIM4_CH1
	*/
	pwm41.set(regv);

	/*
	PC2/ADC12	DMM/VDMM
	*/
	regv = ADC1->JDR3;
	float vdmm = vref * regv / 4096;

	float ohm = vdmm / iout / gain;
	int idr = ~ GPIOD->IDR; //PD8/GPI		DMM/IOPEN
	if(idr & 0x100) ohm = 999999.9;
	return ohm;
}

float o2pt_vget(int erl)
{
	float vo2;
	int regv;

	o2pt_pulse_ms = 0;
	o2pt_rly_set(EIS, 0);
	o2pt_rly_set(EGA, 0);
	o2pt_rly_set(ERL, erl);
	o2pt_mxc_set(BK, GY);

	/*
	PC2/ADC12	DMM/VDMM
	*/
	regv = ADC1->JDR3;
	vo2 = vref * regv / 4096;
	return vo2;
}

void o2pt_pulse(float iout, float hz)
{
	/*
	PB6		VSET_A		TIM4_CH1
	*/
	float df, gm;
	int regv;

	/* iout
	= vref * df * 4.7k / (4.7k + 22k) / 10 ohm
	= vref * df * gm
	=> df = iout / vref / gm
	*/

	gm = 4.7 / (4.7 + 22) / 10.0;
	df = iout / vref / gm;
	regv = (int)(df * 1023);

	/*limit*/
	regv = (regv > 1023) ? 1023 : regv;
	regv = (regv < 0)? 0 : regv;

	//PB6		VSET_A		TIM4_CH1
	pwm41.set(regv);
	o2pt_rly_set(EIS, 1);
	o2pt_rly_set(ERL, 0);
	o2pt_mxc_set(BK, GY);
	o2pt_pulse_pol = 0;

	//avoid switch too fast
	hz = (hz > 50.0f) ? 50.0f : hz;

	o2pt_pulse_ms = (int) (1000.0f / hz);
	o2pt_pulse_timer = time_get(o2pt_pulse_ms);
}

void o2pt_init(void)
{
	o2pt_pulse_ms = 0;
	o2pt_gpio_init();
	o2pt_adc_init();
	o2pt_dac_init();
	o2pt_rly_init();

	buck_set(0, 0);
	o2pt_vget(0);
}

void o2pt_update(void)
{
	if(o2pt_pulse_ms) {
		if(time_left(o2pt_pulse_timer) < 0) {
			o2pt_pulse_timer = time_get(o2pt_pulse_ms);
			o2pt_pulse_pol = !o2pt_pulse_pol;
			if(o2pt_pulse_pol) o2pt_mxc_set(GY, BK);
			else o2pt_mxc_set(BK, GY);
		}
	}
}

int main(void)
{
	sys_init();
	o2pt_init();
	led_flash(LED_GREEN);
	printf("o2pt v2.0, build: %s %s\n\r", __DATE__, __TIME__);

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
		"buck set v	to set buck voltage\n"
		"buck v/i	to readback v or i\n"
		"buck		to readback v and i\n"
	};

	float v, i = -1;

	if(argc > 1) {
		char option = argv[1][0];
		switch(option) {
		case 's':
			if(argc > 2) {
				v = atof(argv[2]);
				buck_set(v, i);
				printf("<+0,OK\n\r");
				return 0;
			}
			break;
		case 'v':
			printf("<%+.3f\n\r", buck_vget());
			return 0;
		case 'i':
			printf("<%+.3f\n\r", buck_iget());
			return 0;
		default:
			break;
		}
	}

	if(argc == 1) {
		v = buck_vget();
		i = buck_iget();
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
		"vopen 		to measure o2s opened voltage\n"
		"vload		to measure o2s loaded voltage\n"
		"pulse i hz	to apply pulsed current on o2s\n"
	};

	float i, f;
	char option = argv[0][1];
	switch(option) {
	case 'o': //vopen
		printf("<%+.3f\n\r", o2pt_vget(0));
		return 0;
	case 'l': //vload
		printf("<%+.3f\n\r", o2pt_vget(1));
		return 0;
	case 'u': //pulse
		if(argc == 3) { //apply pulsed current
			i = atof(argv[1]);
			f = atof(argv[2]);
			o2pt_pulse(i, f);
			printf("<+0,OK\n\r");
			return 0;
		}
	default:
		break;
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


int cmd_key_func(int argc, char *argv[])
{
	int idr = ~ GPIOD->IDR;
	idr &= 0xff;
	printf("<+%d\n\r", idr);
	return 0;
}

const cmd_t cmd_key = {"key", cmd_key_func, "keyboard input monitor"};
DECLARE_SHELL_CMD(cmd_key)

int cmd_rly_func(int argc, char *argv[])
{
	//rly 0 1
	if(argc > 2) {
		int rly = atoi(argv[1]);
		int val = atoi(argv[2]);
		if((rly >= 0) &&(rly <= 15)) {
			o2pt_rly_set(rly, val);
			printf("<+0\n\r");
			return 0;
		}
	}

	printf( \
		"usage:\n" \
		"rly 0 1	switch on rly 0\n" \
	);
	return 0;
}

const cmd_t cmd_rly = {"rly", cmd_rly_func, "relay output drive"};
DECLARE_SHELL_CMD(cmd_rly)

int cmd_res_func(int argc, char *argv[])
{
	if(argc == 5) {
		int apin = atoi(argv[1]);
		int bpin = atoi(argv[2]);
		float i = atof(argv[3]);
		float g = atoi(argv[4]);

		int pass = (apin >= 0) && (apin < 5);
		pass = pass && (bpin >= 0) && (bpin < 5);
		pass = pass && (i > 0) && (i < 0.051);
		pass = pass && ((g == 1) || (g == 100));
		if(pass) {
			float ohm = o2pt_res(apin, bpin, i, g);
			printf("<%+.0f\n\r", ohm);
			return 0;
		}
	}

	printf( \
		"usage:\n" \
		"res pin+ pin- i vgain	measure R(+,-)@i(A), vgain=1 or 100\n" \
		"{0: SHELL, 1: BLACK, 2: GRAY, 3: WHITE1, 4: WHITE2}\n" \
	);
	return 0;
}

const cmd_t cmd_res = {"res", cmd_res_func, "resistor measure"};
DECLARE_SHELL_CMD(cmd_res)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?			to read identification string\n"
		"*RST			instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, www.ulicar.com, %s,%s\n\r", __DATE__, __TIME__);
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
