/*
 * 	feng.ni@2016-10-16  rewrite & simplify
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "stm32f10x.h"
#include "led.h"
#include "dac.h"
#include <stdlib.h>
#include <string.h>
#include "pwm.h"
#include "uart.h"
#include "common/debounce.h"

enum {
	MODE_INIT,
	MODE_O2_LOOP,
	MODE_O2_OPEN,
	MODE_O6_LOOP,
};

static float o2pf_vo2 = -1;
static float o2pf_vga = -1; //external gas alarm signal
static float o2pf_mode = MODE_INIT;
static float o2pf_vth; //unit: V
static float o2pf_rth; //unit: Ohm
static float o2pf_lambda = 0;

void o2pf_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/* LED
	PE14 GPO LED_R
	PE15 GPO LED_G
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* DMM CH SWITCH
	PE8	 GPO MCU_DG_S0
	PE9	 GPO MCU_DG_S1
	PE10 GPO MCU_DG_EN
	PB12 RLY MCU_K_IHEAT
	PB13 RLY MCU_K_IMEN
	PB14 RLY MCU_K_IO2
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* VALVE CONTROL !!!!LDD'S BUG, NOT TIM4, SHOULD PB6/..
	PD8	 LSD MCU_K_LSD0 @TIM4_CH1,AIR
	PD9	 LSD MCU_K_LSD1 @TIM4_CH2,GAS
	PD10 LSD MCU_K_LSD2 @TIM4_CH3,EXT
	PD11 LSD MCU_K_LSD3 @TIM4_CH4,IGN
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	#define valve_ext_off() GPIOD->BRR = GPIO_Pin_10
	#define valve_ext_on() GPIOD->BSRR = GPIO_Pin_10

	/* VHEAT
	PB1  GPO MCU_OFF_BUCK
	PB0  PWM MCU_PWM_BUCK @TIM3_CH3
	PA6  PWM MCU_PWM_HEAT @TIM3_CH1
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	#define vheat_on() GPIOB->BRR = GPIO_Pin_1
	#define vheat_off() GPIOB->BSRR = GPIO_Pin_1

	const pwm_cfg_t pwm_cfg = {.hz = 100000, .fs = 1023};
	pwm31.init(&pwm_cfg);
	#define iheat_pwm_set pwm31.set

	pwm33.init(&pwm_cfg);
	#define vheat_pwm_set pwm33.set

	/* O2-IF
	???  MOS MCU_ENGY
	PC4  MOS MCU_ENR30K
	PC5  MOS MCU_ENR500
	PB15 RLY MCU_K_O2PUMP
	PA4  DAC MCU_DAC0
	PA5  DAC MCU_DAC1
	PC0  ADC MCU_ADC_VO2 @ADC123_IN10
	PC1  ADC MCU_ADC_ALARM @ADC123_IN11
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	#define load_set_500() do {GPIOC->BSRR = GPIO_Pin_5; GPIOC->BRR = GPIO_Pin_4;} while(0)
	#define load_set_30K() do {GPIOC->BSRR = GPIO_Pin_4; GPIOC->BRR = GPIO_Pin_5;} while(0)
	#define pump_vofs_y() GPIOB->BSRR = GPIO_Pin_15
	#define pump_vofs_n() GPIOB->BRR  = GPIO_Pin_15

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	const dac_cfg_t dac_cfg = {.option = DAC_OPT_OBUFOFF};
	dac_ch1.init(&dac_cfg);
	dac_ch2.init(&dac_cfg);

	//ADC INIT, ADC1 INJECTED CH, CONT SCAN MODE
	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz, note: 14MHz at most*/
	ADC_DeInit(ADC1);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_InitStructure.ADC_NbrOfChannel = 0;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_InjectedSequencerLengthConfig(ADC1, 2); //!!!length must be configured at first
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5); //12Mhz / (12.5 + 239.5) = 47Khz
	ADC_InjectedChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_239Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_AutoInjectedConvCmd(ADC1, ENABLE); //!!!must be set because inject channel do not support CONT mode independently

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)); //WARNNING: DEAD LOOP!!!
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	#define adc_vo2_get() (ADC1->JDR1 << 1)
	#define adc_vga_get() (ADC1->JDR2 << 1)

	/*lambda*/
	uart_cfg_t uart_cfg = {.baud = 38400};
	uart2.init(&uart_cfg);
}

void adc_update(void)
{
	float vref = 3.0;
	o2pf_vo2 = adc_vo2_get() * vref / 65536;
	o2pf_vga = adc_vga_get() * vref / 65536;
}

void o2pf_mdelay(float ms)
{
	ms += 0.999;
	time_t deadline = time_get((int) ms);
	while(time_left(deadline) > 0) {
		sys_update();
	}
}

//set o2_yellow voltage
void o2pf_dac0_set(float vset)
{
	float vref = 3.0;
	float gain = 2.0; //100k / 100k
	int ddac = 0;

	ddac = (int) (vset * 4096 / vref / gain);
	ddac = (ddac < 0) ? 0 : ddac;
	ddac = (ddac > 4095) ? 4095 : ddac;

	dac_ch1.write(ddac);
}

//set o2_green/red voltage
void o2pf_dac1_set(float vset)
{
	float vref = 3.0;
	float gain = 2.0; //100k / 100k
	int ddac = 0;

	ddac = (int) (vset * 4096 / vref / gain);
	ddac = (ddac < 0) ? 0 : ddac;
	ddac = (ddac > 4095) ? 4095 : ddac;

	dac_ch2.write(ddac);
}

/*
vout = xlref + is * 10Kohm = vref + vset / 0.47K * 10K
=> vset = ( vout - xlref ) / 21.2766
*/
void o2pf_vheat_set(float v)
{
	float xlref = 1.25;
	float vref = 3.0 * 47 / (100 + 47); //lm4040a

	//fast discharge
	vheat_off();
	o2pf_mdelay(3);
	if(v > 1.5) {
		float vset = (v - xlref) / 21.2766;
		int pwm = (int) (vset * 1024 / vref);
		pwm = (pwm > 1023) ? 1023 : pwm;
		vheat_pwm_set(pwm);
		o2pf_mdelay(3); //2mS in the scope
		vheat_on();
	}
}

//var for open test
static int o2pf_open_ms;
static int o2pf_open_ext; //ext valve status
static time_t o2pf_open_timer;

void o2_open_init(float Ts)
{
	o2pf_mode = MODE_O2_OPEN;
	o2pf_open_ms = (int)(Ts * 1000);
	o2pf_open_ext = 0;
	o2pf_open_timer = time_get(o2pf_open_ms);
	valve_ext_off();
}

void o2_open_update(void)
{
	if(o2pf_mode != MODE_O2_OPEN)
		return;

	if(time_left(o2pf_open_timer) > 0)
		return;

	o2pf_open_timer = time_get(o2pf_open_ms);
	if(o2pf_open_ext == 0) {
		o2pf_open_ext = 1;
		valve_ext_on();
	}
	else {
		o2pf_open_ext = 0;
		valve_ext_off();
	}
}

struct debounce_s o2pf_mon_vo2;

void o2_loop_init(float vth)
{
	o2pf_mode = MODE_O2_LOOP;
	o2pf_vth = vth;

	//config valve-ext to on/off mode
	//PD10 LSD MCU_K_LSD2 @TIM4_CH3,EXT
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//init providing vo2 is lower than 0.45v, which indicates currently valve ext is on
	debounce_t_init(&o2pf_mon_vo2, 10, 0); //10mS, low level
	valve_ext_off(); //vo2 is low, turn off ext
}

void o2_loop_update(void)
{
	if(o2pf_mode != MODE_O2_LOOP)
		return;

	int event = debounce(&o2pf_mon_vo2, o2pf_vo2 > o2pf_vth);
	if(event) {
		if (o2pf_mon_vo2.on) valve_ext_on(); //vo2 is high, turn on ext
		else valve_ext_off(); //vo2 is low, turn off ext
	}
}

void o6_loop_init(float vth, float rth)
{
	o2pf_mode = MODE_O6_LOOP;
	o2pf_vth = vth;
	o2pf_rth = rth;
}

void o6_loop_update(void)
{
	if(o2pf_mode != MODE_O6_LOOP)
		return;
}

void o2pf_update(void)
{
	adc_update();
	o2_loop_update();
	o2_open_update();
	o6_loop_update();
}

static int lambda_cmd = 0;
static int lambda_deadline;
static int lambda_msb;
static int lambda_lsb;

void lambda_init(void)
{
	lambda_deadline = time_get(20);
	lambda_msb = lambda_lsb = 0;
	
	//dump uart2 input fifo
	while(uart2.poll() > 0) {
		uart2.getchar();
	}

	lambda_cmd = 2;
	uart2.putchar(lambda_cmd);
}

void lambda_update(void)
{
	if(time_left(lambda_deadline) < 0) {
		//timeout .. :(
		lambda_init();
		o2pf_lambda = 0;
	}

	if(uart2.poll() > 0) {
		//received response from etas
		if(lambda_cmd == 2) {
			//msb received
			lambda_msb = uart2.getchar();
			lambda_cmd = 3;
			uart2.putchar(lambda_cmd);
		}
		else {
			//lsb received
			lambda_lsb = uart2.getchar();
			o2pf_lambda = (lambda_msb * 256 + lambda_lsb) / 1000.0;
			lambda_init();
		}
	}
}

void main(void)
{
	o2pf_init();
	sys_init();
	
	/* do not r/w es630.1 during it's powering-up !!!
	or it's smb communication will die
	*/
	//lambda_init(); 
	while(1) {
		sys_update();
		o2pf_update();
		if(lambda_cmd > 0) lambda_update();
	}
}

static int cmd_dmm_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dmm ch_name	ch_name = iout/iheat/vout/vheat/vdac0/vtemp\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	/*
	PB13 RLY MCU_K_IMEN
	PB14 RLY MCU_K_IO2
	PB12 RLY MCU_K_IHEAT
	PE10 GPO MCU_DG_EN
	PE9	 GPO MCU_DG_S1
	PE8	 GPO MCU_DG_S0 ... bit 0
	*/

	struct dmm_ch_s {
		const char *ch_name;
		int ioset;
	};

	const struct dmm_ch_s dmm_ch_list[] = {
		{.ioset = 0x30/*0b110000*/, .ch_name = "iout"},
		{.ioset = 0x28/*0b101000*/, .ch_name = "iheat"},
		{.ioset = 0x04/*0b000100*/, .ch_name = "vout"},
		{.ioset = 0x05/*0b000101*/, .ch_name = "vdac0"},
		{.ioset = 0x06/*0b000110*/, .ch_name = "vheat"},
		{.ioset = 0x07/*0b000111*/, .ch_name = "vtemp"},
	};

	int ecode = -1;
	int ioset = 0;
	for(int i = 0; i < 6; i ++) {
		if (!strcmp(argv[1], dmm_ch_list[i].ch_name)) {
			//found it
			ioset = dmm_ch_list[i].ioset;
			ecode = 0;
		}
	}

	if(ecode) {
		printf("<%+d, ch name not identified!\n", ecode);
		return 0;
	}

	#define MCU_K_IHEAT GPIO_Pin_12
	#define MCU_K_IMEN GPIO_Pin_13
	#define MCU_K_IO2 GPIO_Pin_14
	GPIOB->BRR = MCU_K_IMEN | MCU_K_IO2 | MCU_K_IHEAT;
	o2pf_mdelay(20);
	if(ioset & (1 << 5)) GPIOB->BSRR = MCU_K_IMEN;
	if(ioset & (1 << 4)) GPIOB->BSRR = MCU_K_IO2;
	if(ioset & (1 << 3)) GPIOB->BSRR = MCU_K_IHEAT;
	o2pf_mdelay(20);

	#define MCU_DG_EN GPIO_Pin_10
	#define MCU_DG_S1 GPIO_Pin_9
	#define MCU_DG_S0 GPIO_Pin_8
	GPIOE->BRR = MCU_DG_EN | MCU_DG_S1 | MCU_DG_S0;
	if(ioset & (1 << 2)) GPIOE->BSRR = MCU_DG_EN;
	if(ioset & (1 << 1)) GPIOE->BSRR = MCU_DG_S1;
	if(ioset & (1 << 0)) GPIOE->BSRR = MCU_DG_S0;
	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "o2pf dmm ch switch commands"};
DECLARE_SHELL_CMD(cmd_dmm)

static int cmd_power_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"power vdac0 1.0		ch_name=vdac0/vdac1/vheat\n"
	};

	if(argc < 3) {
		printf("%s", usage);
		return 0;
	}

	//const char *ch_name = argv[1];
	float v = 0.0;
	int n = sscanf(argv[2], "%f", &v);
	if(n != 1) {
		printf("<-2, voltage error\n");
		return 0;
	}

	switch(argv[1][4]) {
	case '0': //vdac0
		o2pf_dac0_set(v);
		break;
	case '1': //vdac1
		o2pf_dac1_set(v);
		break;
	case 't': //vheat
		o2pf_vheat_set(v);
		break;
	default:
		printf("<-1, ch name not identified\n");
		return 0;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_power = {"power", cmd_power_func, "o2pf power setting commands"};
DECLARE_SHELL_CMD(cmd_power)

static int cmd_valve_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"valve air 1/0		valve_name=ign/ext/gas/air\n"
		"valve ext 0.5 100	pwm mode, 50% @ 100hz\n"
		"valve ?			valve status(ign|ext|gas|air)\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	const char *ch_name = argv[1];
	float v = 0.0, hz = -1;
	int status = 0;
	if(argc > 2) {
		int n = sscanf(argv[2], "%f", &v);
		if(n != 1) {
			printf("<-2, value error\n");
			return 0;
		}
		if(argc == 4) {
			n = sscanf(argv[3], "%f", &hz);
			if((n != 1) || (hz <= 0.01)) {
				printf("<-3, pwm freq error\n");
				return 0;
			}
		}
	}

	const pwm_bus_t *pwm_ch = NULL;
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	/* VALVE CONTROL
	PD8	 LSD MCU_K_LSD0 @TIM4_CH1,AIR
	PD9	 LSD MCU_K_LSD1 @TIM4_CH2,GAS
	PD10 LSD MCU_K_LSD2 @TIM4_CH3,EXT
	PD11 LSD MCU_K_LSD3 @TIM4_CH4,IGN
	*/

	switch(ch_name[0]) {
	case 'a': //air
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
		pwm_ch = &pwm41;
		break;
	case 'g': //gas
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		pwm_ch = &pwm42;
		break;
	case 'e': //ext
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		pwm_ch = &pwm43;
		break;
	case 'i': //ign
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		pwm_ch = &pwm44;
		break;
	case '?':
		status = GPIOD->ODR;
		status >>= 8;
		printf("<%+d, OK\n", status);
		return 0;
	default:
		printf("<-1, ch name not identified\n");
		return 0;
	}

	if(hz < 0) {
		//valve on/off ctrl
		GPIO_Init(GPIOD, &GPIO_InitStructure);
		if(v > 0.5) GPIOD->BSRR = GPIO_InitStructure.GPIO_Pin;
		else GPIOD->BRR = GPIO_InitStructure.GPIO_Pin;
	}
	else {
		//pwm control
		int df = (int)(v * 1000);
		df = (df > 65535) ? 65535 : df;
		df = (df < 0) ? 0 : df;

		pwm_cfg_t pwm_cfg = {.hz = hz, .fs = 999};
		pwm_ch -> init(&pwm_cfg);
		pwm_ch -> set(df);
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_valve = {"valve", cmd_valve_func, "o2pf valve setting commands"};
DECLARE_SHELL_CMD(cmd_valve)

static int cmd_lambda_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"lambda	?		read current lambda value\n"
		"lambda x		cancel lambda_update\n"
		"lambda ri		query ri, return 0.0~250ohm\n"
		"lambda ip		query ip, return -3~3mA\n"
		"lambda 0x01		query etas lambda meter@addr=0\n"
		"			1: 8bit lambda, (byte+186)/250\n"
		"			2: msb of lambda, 0.7~32.767\n"
		"			3: lsb of lambda, (msb*256+lsb)/1000\n"
		"			4: msb of Ri, 0.0~250.0 Ohm\n"
		"			5: lsb of Ri, (msb*256+lsb)/10\n"
		"			6: msb of O2, 0.0~24.41%\n"
		"			7: lsb of O2, (msb*256+lsb)/10\n"
		"			8: msb of A/F, 10.29~327.67\n"
		"			9: lsb of A/F, (msb*256+lsb)/100\n"
		"			c: msb of Ip, -3mA~3mA\n"
		"			d: lsb of Ip, (msb*256+lsb)/10\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argv[1][0] == '?') {
		if(lambda_cmd < 1) {
			lambda_init();
		}
		printf("<%+.3f, ok\n", o2pf_lambda);
		return 0;
	}
	else if(argv[1][0] == 'x') {
		lambda_cmd = 0;
		return 0;
	}
	else {
		int cmd, n;
		n = sscanf(argv[1], "%x", &cmd);
		if(n == 1) {
			uart2.putchar(cmd);
			time_t deadline = time_get(100);
			while (time_left(deadline) > 0) {
				sys_update();
				if(uart2.poll() > 0) {
					int echo = uart2.getchar();
					printf("<0x%02x, ok\n", echo);
					return 0;
				}
			}
			printf("<-2, etas no response\n");
			return 0;
		}
	}
	printf("<-1, cmd para invalid\n");
	return 0;
}

cmd_t cmd_lambda = {"lambda", cmd_lambda_func, "etas lambda meter i/f commands"};
DECLARE_SHELL_CMD(cmd_lambda)

static int cmd_o2if_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"o2if vo2		read o2 sensor output voltage\n"
		"o2if vga		read ga(gas alarm) signal voltage\n"
		"o2if iheat 0.5  	df = 50% @ 72khz\n"
		"o2if load 500		vo2 load RL = 500 ohm, pick a near one\n"
		"o2if vofs y/n		vpump vofset on/off, typ: 56K@1V8\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	int df, delta1, delta2, n = strlen(argv[1]);
	char key = argv[1][n - 1]; //last char
	float v = 0.0;

	if(argc > 2) {
		if(argv[2][0] == 'y') v = 1;
		else if(argv[2][0] == 'n') v = 0;
		else {
			n = sscanf(argv[2], "%f", &v);
			if(n != 1) {
				printf("<-2, value error\n");
				return 0;
			}
		}
	}

	switch(key) {
	case '2': //vo2
		printf("<%+.3f, vo2(unit: V)\n", o2pf_vo2);
		return 0;
	case 'a': //vga, gas alarm
		printf("<%+.3f, vga(unit: V)\n", o2pf_vga);
		return 0;
	case 't': //iheat
		df = (int) (v * 1024);
		df = (df > 1023) ? 1023 : df;
		df = (df < 0) ? 0 : df;
		iheat_pwm_set(df);
		break;
	case 'd': //load
		delta1 = abs(500 - (int) v);
		delta2 = abs(30000 - (int) v);
		if(delta1 < delta2) load_set_500();
		else load_set_30K();
		break;
	case 's': //vofs
		if(v > 0.5) pump_vofs_y();
		else pump_vofs_n();
		break;
	default:
		printf("<-1, sub cmd not supported\n");
		return 0;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_o2if = {"o2if", cmd_o2if_func, "o2pf oxgen sensor i/f commands"};
DECLARE_SHELL_CMD(cmd_o2if)

static int cmd_mode_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mode init			init o2pf run mode\n"
		"mode open 2		enable o2 open test mode, T = 2s\n"
		"mode loop 0.45		enable o2 closed loop test mode\n"
		"mode o6lp 0.45		enable o6 closed loop mode\n"
		"mode o6lp 0.45 300		also closed loop of ri = 300ohm\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	int n;
	char key = argv[1][1];
	float v, T, vth;
	float rth = -1.0;

	if(argc > 2) {
		n = sscanf(argv[2], "%f", &v);
		if(n != 1) {
			printf("<-2, value error\n");
			return 0;
		}

		if(argc > 3) {
			n = sscanf(argv[3], "%f", &rth);
			if(n != 1) {
				printf("<-3, para ri error\n");
				return 0;
			}
		}
	}

	switch(key) {
	case 'n': //init
		o2pf_mode = MODE_INIT;
		break;
	case 'p': //o2 open test mode
		T = (argc > 2) ? v : 2.0; //unit: S
		o2_open_init(T);
		break;
	case 'o': //o2 loop test mode
		vth = (argc > 2) ? v : 0.45; //unit: V
		o2_loop_init(vth);
		break;
	case '6': //o6lp mode
		vth = (argc > 2) ? v : 0.45; //unit: V
		o6_loop_init(vth, rth);
		break;
	default:
		printf("<-1, sub cmd not supported\n");
		return 0;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_mode = {"mode", cmd_mode_func, "o2pf work mode switch"};
DECLARE_SHELL_CMD(cmd_mode)
