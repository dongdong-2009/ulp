/*
 * 	feng.ni@2016-10-16  rewrite & simplify
 * test mode:
 * idle
 * open ext valve on/off per 2S, capture if dmm trig is enabled
 * loop ext valve on/off by vo2, capture if dmm trig is enabled
 * warning:
 * 1, o2/o6 must be config during idle, or refused!
 * 2, dmm trig must be config during idle, or refused!
 * 3, o6 do not support loop test mode!
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

#define CONFIG_DMM_TRIG 0
#define CONFIG_PUMP_EN 0 //hw v1.2 not support when r11 not exist

#define GAS_ALARM_WARN_MS 3000
#define GAS_ALARM_OFF_MS 15000
#define EXT_LOOP_DITHER_MS 10
#define EXT_PWM_HZ_DEF 100

static float o2pf_vga; //external gas alarm signal
static float o2pf_vot; //external temperature signal
static float o2pf_vo2; //oxgen sensor output voltage, unit: V
static float o2pf_ohm; //oxgen sensor output resistence, unit: Ohm
static float o2pf_lambda;
static float o2pf_lmtest; //average lamb durint last test

static float o2pf_avg_l; //average lamb calculator
static int o2pf_avg_t; //last lamb capture time, unit: mS
static time_t o2pf_avg_timer; //average lamb start time

struct o2pf_config_s {
	//global flag
	unsigned gas_alarm_disable : 1;
	unsigned trig : 1; //enable dmm_isr
	unsigned test : 1; //enable ext_isr
	unsigned open : 1; //ext_isr = opentest mode
	unsigned loop : 1; //ext_isr = looptest mode
	unsigned lamb : 1; //ext_isr = lambtest mode
	unsigned o6   : 1; //enable o6_isr

	//dmm trig config
	int Ts : 10; //sampling interval, unit: mS, range: 1 - 1000

	//test config
	int vpw; //valve period width, unit: mS, range: 10mS - 10S
	float vdf; //valve duty factor, only for lambda test
	float vth; //valve threshold, only for looptest

	//o6 config
	float o6v; //o6 closed loop oxgen output voltage
	float o6r; //o6 closed loop oxgen output resistence
} o2pf_config;

enum {
	VALVE_AIR = 0,
	VALVE_GAS,
	VALVE_EXT,
	VALVE_IGN, /*it is used as gas alarm output really*/
	ALARM_GAS = VALVE_IGN,
};

static void o2pf_mdelay(float ms);

/* VALVE CONTROL
PC6	LSD MCU_K_AIR @TIM8_CH1,AIR
PC7	LSD MCU_K_GAS @TIM8_CH2,GAS
PC8 LSD MCU_K_EXT @TIM8_CH3,EXT
PC9 LSD MCU_K_IGN @TIM8_CH4,IGN
*/

const uint16_t valve_gpos[] = {
	GPIO_Pin_6,
	GPIO_Pin_7,
	GPIO_Pin_8,
	GPIO_Pin_9,
};

const pwm_bus_t *valve_pwms[] = {
	&pwm81,
	&pwm82,
	&pwm83,
	&pwm84,
};

void valve_gpo_init(int valve, int init_lvl)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = valve_gpos[valve];
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	if(init_lvl > 0) GPIOC->BSRR = GPIO_InitStructure.GPIO_Pin;
	else GPIOC->BRR = GPIO_InitStructure.GPIO_Pin;

	#define valve_air_off() GPIOC->BRR = GPIO_Pin_6
	#define valve_air_on() GPIOC->BSRR = GPIO_Pin_6
	#define valve_gas_off() GPIOC->BRR = GPIO_Pin_7
	#define valve_gas_on() GPIOC->BSRR = GPIO_Pin_7
	#define valve_ext_off() GPIOC->BRR = GPIO_Pin_8
	#define valve_ext_on() GPIOC->BSRR = GPIO_Pin_8

	/*ign is used as gas alarm output really*/
	#define valve_alarm_off() GPIOC->BRR = GPIO_Pin_9
	#define valve_alarm_on() GPIOC->BSRR = GPIO_Pin_9

	//valve status(ign|ext|gas|air)
	#define valve_status() ((GPIOC->IDR >> 6) & 0x0f)
	#define valve_gas_status() (GPIOC->IDR & GPIO_Pin_7)
}

void valve_pwm_set(int valve, float df)
{
	const pwm_bus_t * pwm_ch = valve_pwms[valve];

	int regv = (int)(df * 10000);
	regv = (regv > 9999) ? 9999 : regv;
	regv = (regv < 0) ? 0 : regv;
	pwm_ch -> set(regv);
}

void valve_pwm_init(int valve, float df, float hz)
{
	const pwm_bus_t * pwm_ch = valve_pwms[valve];
	if(hz > 0) {
		pwm_cfg_t pwm_cfg = {.hz = hz, .fs = 9999};
		pwm_ch -> init(&pwm_cfg);
	}
	else {
		//do not init main timer, maybe shared by pwm channels
		pwm_ch -> init(NULL);
	}

	valve_pwm_set(valve, df);
}

/*
PA6  PWM MCU_PWM_HEAT @TIM3_CH1
*/
void iheat_mode_gpo(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIOA->BRR = GPIO_Pin_6;
}

void iheat_mode_pwm(void) {
	pwm31.init(NULL);
	pwm31.set(0);
}

void o2pf_bsp_init(void)
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

	PE11 GPO MCU_TRIG
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	#define MCU_DG_S0(lvl) do {if(lvl) GPIOE->BSRR = GPIO_Pin_8; else GPIOE->BRR = GPIO_Pin_8;} while(0)
	#define MCU_DG_S1(lvl) do {if(lvl) GPIOE->BSRR = GPIO_Pin_9; else GPIOE->BRR = GPIO_Pin_9;} while(0)
	#define MCU_DG_EN(lvl) do {if(lvl) GPIOE->BSRR = GPIO_Pin_10; else GPIOE->BRR = GPIO_Pin_10;} while(0)

	//down going edge trig
	#define MCU_TRIG(yes) do {if(yes) GPIOE->BRR = GPIO_Pin_11; else GPIOE->BSRR = GPIO_Pin_11;} while(0)

	#define MCU_K_IHEAT(lvl) do {if(lvl) GPIOB->BSRR = GPIO_Pin_12; else GPIOB->BRR = GPIO_Pin_12;} while(0)
	#define MCU_K_IMEN(lvl) do {if(lvl) GPIOB->BSRR = GPIO_Pin_13; else GPIOB->BRR = GPIO_Pin_13;} while(0)
	#define MCU_K_IO2(lvl) do {if(lvl) GPIOB->BSRR = GPIO_Pin_14; else GPIOB->BRR = GPIO_Pin_14;} while(0)

	/* VALVE CONTROL*/
	valve_gpo_init(VALVE_AIR, 0);
	valve_gpo_init(VALVE_GAS, 0);
	valve_gpo_init(ALARM_GAS, 0);

	//to init timer base
	valve_pwm_init(VALVE_EXT, 0.5, EXT_PWM_HZ_DEF);
	valve_gpo_init(VALVE_EXT, 0);

	/* VHEAT
	PB1  GPO MCU_OFF_BUCK
	PA6  PWM MCU_PWM_HEAT @TIM3_CH1 IHEAT
	PA7  PWM MCU_PWM_O6IS @TIM3_CH2 ILOAD
	PB0  PWM MCU_PWM_BUCK @TIM3_CH3 VHEAT
	*/
	#define vheat_y() GPIOB->BRR = GPIO_Pin_1
	#define vheat_n() GPIOB->BSRR = GPIO_Pin_1
	#define iheat_y() GPIOA->BSRR = GPIO_Pin_6
	#define iheat_n() GPIOA->BRR = GPIO_Pin_6

	#define iheat_pwm_set pwm31.set
	#define iload_pwm_set pwm32.set
	#define vheat_pwm_set pwm33.set

	//vheat on/off
	const pwm_cfg_t pwm_cfg = {.hz = 100000, .fs = 1023};
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	vheat_n();
	pwm33.init(&pwm_cfg); //vheat/buck

	//iheat
	iheat_mode_gpo();
	iheat_y();

	//iload
	pwm32.init(&pwm_cfg);
	iload_pwm_set(0);

	/* O2-IF
	PB10 RLY MCU_K_ENO6
	PB11 MOS MCU_ENR500
	PB15 RLY MCU_K_O2PUMP

	PA4  DAC MCU_DAC0
	PA5  DAC MCU_DAC1
	PC0  ADC MCU_ADC_VO2 @ADC123_IN10
	PC1  ADC MCU_ADC_VGA @ADC123_IN11
	PC2  ADC MCU_ADC_VOT @ADC123_IN12
	*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	#define load_500R_y() GPIOB->BSRR = GPIO_Pin_11
	#define load_500R_n() GPIOB->BRR = GPIO_Pin_11
	#define load_30kR_y() GPIOB->BRR  = GPIO_Pin_15
	#define load_30kR_n() GPIOB->BSRR = GPIO_Pin_15

#if CONFIG_PUMP_EN == 1
	#define pump_vofs_y() GPIOB->BSRR = GPIO_Pin_15
	#define pump_vofs_n() GPIOB->BRR  = GPIO_Pin_15
#endif

	#define mcu_vo2_y() GPIOB->BSRR = GPIO_Pin_10
	#define mcu_vo2_n() GPIOB->BRR = GPIO_Pin_10

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
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

	#define adc_vo2_get() (ADC1->JDR1 << 1)
	#define adc_vga_get() (ADC1->JDR2 << 1)
	#define adc_vot_get() (ADC1->JDR3 << 1)

	/*lambda*/
	uart_cfg_t uart_cfg = {.baud = 38400};
	uart2.init(&uart_cfg);
}

void o2pf_bsp_update(void)
{
	float vref = 3.0;
	o2pf_vo2 = adc_vo2_get() * vref / 65536;
	o2pf_vga = adc_vga_get() * vref / 65536;
	o2pf_vot = adc_vot_get() * vref / 65536;

	if(o2pf_config.gas_alarm_disable)
		return;

	//gas alarm
	static time_t gas_alarm_timer;
	if( valve_gas_status() ) {
		if(gas_alarm_timer == 0) {
			//warn & off timer
			gas_alarm_timer = time_get(GAS_ALARM_WARN_MS);
		}
		else if(time_left(gas_alarm_timer) < GAS_ALARM_WARN_MS - GAS_ALARM_OFF_MS) {
			//auto turn off air&gas valve
			valve_gas_off();
			valve_air_off();
		}
		else if(time_left(gas_alarm_timer) < 0) {
			//turn on gas alarm
			valve_alarm_on();
		}
		else { // time_left(gas_alarm_timer) > 0
		}
	}
	else {
		gas_alarm_timer = 0;
		valve_alarm_off();
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
	vheat_n();
	o2pf_mdelay(3);
	if(v > 1.5) {
		float vset = (v - xlref) / 21.2766;
		int pwm = (int) (vset * 1024 / vref);
		pwm = (pwm > 1023) ? 1023 : pwm;
		vheat_pwm_set(pwm);
		o2pf_mdelay(3); //2mS in the scope
		vheat_y();
	}
}

enum {
	LAMBDA_CMD_NONE = 0,
	LAMBDA_RD_MSB = 2,
	LAMBDA_RD_LSB = 3,
};

static int lambda_cmd = 0;
static int lambda_deadline;
static int lambda_msb;
static int lambda_lsb;

void lambda_init(int cmd)
{
	/* do not r/w es630.1 during it's powering-up !!!
	or it's smb communication will die
	*/
	lambda_msb = lambda_lsb = 0;
	lambda_cmd = cmd;

	if(cmd > 0) {
		//dump uart2 input fifo
		while(uart2.poll() > 0) {
			uart2.getchar();
		}

		lambda_deadline = time_get(20);
		uart2.putchar(lambda_cmd);
	}
}

void lambda_update(void)
{
	if(lambda_cmd == 0)
		return;

	if(time_left(lambda_deadline) < 0) {
		//timeout .. :(
		lambda_init(LAMBDA_RD_MSB);
		o2pf_lambda = 0;
	}

	if(uart2.poll() > 0) {
		//received response from etas
		if(lambda_cmd == LAMBDA_RD_MSB) {
			//msb received
			lambda_msb = uart2.getchar();
			lambda_cmd = LAMBDA_RD_LSB;
			uart2.putchar(lambda_cmd);
		}
		else {
			//lsb received
			lambda_lsb = uart2.getchar();
			o2pf_lambda = (lambda_msb * 256 + lambda_lsb) / 1000.0;
			if(o2pf_avg_timer != 0) { //to calculate average lamb during test
				int t = - time_left(o2pf_avg_timer);
				o2pf_avg_l = (o2pf_avg_l * o2pf_avg_t + o2pf_lambda * (t - o2pf_avg_t)) / t ;
				o2pf_avg_t = t;
			}
			lambda_init(LAMBDA_RD_MSB);
		}
	}
}

struct dmm_s {
	time_t timer;
} o2pf_dmm;

void dmm_isr(void)
{
#if CONFIG_DMM_TRIG == 1
	MCU_TRIG(0);
	if(!o2pf_config.trig) return;
	if(time_left(o2pf_dmm.timer) <= 0) {
		o2pf_dmm.timer = time_get(o2pf_config.Ts);
		MCU_TRIG(1);
	}
#endif
}

struct ext_s {
	time_t timer;
	float df;
	int on;
} o2pf_ext;

//valve-ext auto switch
void ext_isr(void)
{
	if(!o2pf_config.test) return;

	if(o2pf_config.open) {
		if (time_left(o2pf_ext.timer) < 0) {
			o2pf_ext.timer = time_get(o2pf_config.vpw);
			o2pf_ext.on = ! o2pf_ext.on;
		}

		if(o2pf_ext.on) valve_ext_on();
		else valve_ext_off();
	}

	if(o2pf_config.loop) {
		if(time_left(o2pf_ext.timer) < 0) { //noise eject
			int on = (o2pf_vo2 < o2pf_config.vth) ? 0 : 1; //vo2 low => off ext valve
			if (on != o2pf_ext.on) {
				o2pf_ext.on = on;
				o2pf_ext.timer = time_get(EXT_LOOP_DITHER_MS);
			}
		}

		if(o2pf_ext.on) valve_ext_on();
		else valve_ext_off();
	}

	if(o2pf_config.lamb) {
		if (time_left(o2pf_ext.timer) < 0) {
			o2pf_ext.timer = time_get(o2pf_config.vpw);
			o2pf_ext.df += o2pf_config.vdf;
			if(o2pf_ext.df > 1.0) o2pf_ext.df = o2pf_config.vdf;
			valve_pwm_set(VALVE_EXT, o2pf_ext.df);
		}
	}
}

void o6_isr(void)
{
}

//auto-called by sys timer isr per-1mS
void sys_tick(void) {
	o6_isr();
	dmm_isr();
	ext_isr();
}

void o2pf_init(void)
{
	o2pf_vga = 0;
	o2pf_vot = 0;
	o2pf_vo2 = 0;
	o2pf_ohm = 0;
	o2pf_lambda = 0;

	o2pf_config.gas_alarm_disable = 0;
	o2pf_config.test = 0;
	o2pf_config.o6 = 0;

	o2pf_config.Ts = 100;
	o2pf_config.vpw = 2000;
	o2pf_config.vth = 0.45;
	o2pf_config.o6v = 0.50;
	o2pf_config.o6r = 300.0;


	o2pf_bsp_init();
	lambda_init(LAMBDA_CMD_NONE);
	valve_alarm_off();
}

void o2pf_update(void)
{
	o2pf_bsp_update();
	lambda_update();
}

void o2pf_mdelay(float ms)
{
	ms += 0.999;
	time_t deadline = time_get((int) ms);
	while(time_left(deadline) > 0) {
		sys_update();
	}
}

void main(void)
{
	o2pf_init();
	sys_init();

	while(1) {
		sys_update();
		o2pf_update();
	}
}

static int cmd_dmm_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dmm reset		stop dmm auto trig, then init dmm related relays\n"
		"dmm start		start dmm auto trig\n"
		"dmm ch_name [Ts]	ch_name = iout/iheat/vout/vheat/vdac0/vtemp\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc > 2) {
		float Ts = 0.1;
		int n = sscanf(argv[2], "%f", &Ts);
		if((n != 1) || (Ts < 0.01)) {
			printf("<-1, cmd para error\n");
			return 0;
		}
		o2pf_config.Ts = (int)(Ts * 1000);
		o2pf_dmm.timer = time_get(0);
	}

	if(strcmp(argv[1], "start") == 0) {
		o2pf_config.trig = 1;
	}
	else if(strcmp(argv[1], "reset") == 0) {
		o2pf_config.trig = 0;
		MCU_K_IMEN(0);
		MCU_K_IHEAT(0);
		MCU_K_IO2(0);
		MCU_DG_EN(0);
	}
	else if(strcmp(argv[1], "iout") == 0) {
		MCU_K_IMEN(1);
		MCU_K_IHEAT(0);
		MCU_K_IO2(1);
	}
	else if(strcmp(argv[1], "iheat") == 0) {
		MCU_K_IMEN(1);
		MCU_K_IO2(0);
		MCU_K_IHEAT(1);
	}
	else if(strcmp(argv[1], "vout") == 0) {
		MCU_K_IMEN(0);
		MCU_DG_S1(0);
		MCU_DG_S0(0);
		MCU_DG_EN(1);
	}
	else if(strcmp(argv[1], "vheat") == 0) {
		MCU_K_IMEN(0);
		MCU_DG_S1(1);
		MCU_DG_S0(0);
		MCU_DG_EN(1);
	}
	else if(strcmp(argv[1], "vdac0") == 0) {
		MCU_K_IMEN(0);
		MCU_DG_S1(0);
		MCU_DG_S0(1);
		MCU_DG_EN(1);
	}
	else if(strcmp(argv[1], "vtemp") == 0) {
		MCU_K_IMEN(0);
		MCU_DG_S1(1);
		MCU_DG_S0(1);
		MCU_DG_EN(1);
	}
	else {
		printf("<%+d, ch name not identified!\n", -2);
		return 0;
	}

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
		"valve air y/n		valve_name=ign/ext/gas/air\n"
		"valve ext 0.5 [hz]	pwm mode, 50% @ hz\n"
		"valve ?			valve status(ign|ext|gas|air)\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	const char *ch_name = argv[1];
	float v = 0.0, hz = -1;
	int status = 0;
	int mode_gpio = 1;

	if(argc > 2) {
		if(argv[2][0] == 'y') v = 1.0;
		else if(argv[2][0] == 'n') v = 0.0;
		else {
			mode_gpio = 0;

			int n = sscanf(argv[2], "%f", &v);
			if((n == 1) && (argc == 4)) {
				n = sscanf(argv[3], "%f", &hz);
			}

			if(n != 1) {
				printf("<-2, cmd para error\n");
				return 0;
			}
		}
	}

	int valve = -1;
	switch(ch_name[0]) {
	case 'a': //air
		valve = VALVE_AIR;
		break;
	case 'g': //gas
		valve = VALVE_GAS;
		break;
	case 'e': //ext
		valve = VALVE_EXT;
		break;
	case 'i': //ign, for debug purpose
		valve = VALVE_IGN;
		break;
	case '?':
		status = valve_status();
		printf("<%+d, OK\n", status);
		return 0;
	default:
		printf("<-1, ch name not identified\n");
		return 0;
	}

	if(mode_gpio) valve_gpo_init(valve, (int) v);
	else valve_pwm_init(valve, v, hz);
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
		"lambda a		read average lambda\n"
		"lambda t		read average lambda of last test\n"
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
		if(lambda_cmd == LAMBDA_CMD_NONE) {
			lambda_init(LAMBDA_RD_MSB);
		}
		printf("<%+.3f, ok\n", o2pf_lambda);
		return 0;
	}
	else if(argv[1][0] == 'a') {
		printf("<%+.3f, ok\n", o2pf_avg_l);
		return 0;
	}
	else if(argv[1][0] == 't') {
		printf("<%+.3f, ok\n", o2pf_lmtest);
		return 0;
	}
	else if(argv[1][0] == 'x') {
		lambda_cmd = LAMBDA_CMD_NONE;
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
		"o2if vot		read Vot(oxgen sensor temperature)\n"
		"o2if load 500		vo2 load RL = 500 ohm, pick a near one\n"
		"o2if pump y/n		pump relay on/off\n"
		"o2if alarm y/n		gas alarm enable or disable\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	int n = strlen(argv[1]);
	char key = argv[1][n - 1]; //last char
	float v = 0.0;

	if(argc > 2) {
		if(argv[2][0] == 'y') v = 1;
		else if(argv[2][0] == 'n') v = 0;
		else {
			n = sscanf(argv[2], "%f", &v);
			if(n != 1) {
				printf("<-2, cmd para error\n");
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
	case 't': //vot, oxgen temperature
		printf("<%+.3f, vot(unit: V)\n", o2pf_vot);
		return 0;
	case 'm':
		if(v > 0.5) o2pf_config.gas_alarm_disable = 0; //enable
		else o2pf_config.gas_alarm_disable = 1;
		break;
	case 'd': //load
		if(o2pf_config.test) {
			printf("<-3, test is busy\n");
			return 0;
		}
		if((v < 1.0) || (v > 1.0e6)) { //open load
			load_500R_n();
			load_30kR_n();
		}
		else {
			int rset = (int) v;
			int delta_500 = (rset > 500) ? (rset - 500) : (500 - rset);
			int delta_30k = (rset > 30000) ? (rset - 30000) : (30000 - rset);
			if(delta_500 < delta_30k) {
				load_500R_y();
				load_30kR_n();
			}
			else {
				load_500R_n();
				load_30kR_y();
			}
		}
		break;
#if CONFIG_PUMP_EN == 1
	case 'p': //pump
		if(o2pf_config.test) {
			printf("<-3, test is busy\n");
			return 0;
		}
		if(v > 0.5) pump_vofs_y();
		else pump_vofs_n();
		break;
#endif
	default:
		printf("<-1, sub cmd not supported\n");
		return 0;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_o2if = {"o2if", cmd_o2if_func, "o2pf oxgen sensor i/f commands"};
DECLARE_SHELL_CMD(cmd_o2if)

static int cmd_o6if_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"o6if 0.45 [ri]		closed loop para setting\n"
		"o6if y/n		to enable/disable o6 loop\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(o2pf_config.test) {
		printf("<-2, test is busy\n");
		return 0;
	}

	int n;
	float v, r = 0.0;

	switch(argv[1][0]) {
	case 'y':
		mcu_vo2_y();
		o2pf_config.o6 = 1;
		break;
	case 'n':
		mcu_vo2_n();
		o2pf_config.o6 = 0;
		break;
	default:
		n = sscanf(argv[1], "%f", &v);
		if((n == 1) && (argc > 2)) {
			n = sscanf(argv[2], "%f", &r);
		}
		if(n != 1) {
			printf("<-1, cmd para error\n");
			return 0;
		}

		o2pf_config.o6v = v;
		o2pf_config.o6r = r;
		break;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_o6if = {"o6if", cmd_o6if_func, "6-lines oxgen sensor i/f commands"};
DECLARE_SHELL_CMD(cmd_o6if)

static int cmd_test_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"test reset			reset any test mode & stop dmm trig\n"
		"test open 2		start open loop test, T = 2s * 2\n"
		"test loop 0.45		start closed loop test, vtrig = 0.45v\n"
		"test lamb 5 0.1  start static lambda test, step=5s, df=0.1*N\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	float v, vdf = 0.1;
	if(argc > 2) {
		int n = sscanf(argv[2], "%f", &v);
		if(n == 0) {
			printf("<-1, cmd para error\n");
			return 0;
		}
	}

	if(argc > 3) {
		int n = sscanf(argv[2], "%f", &vdf);
		if(n == 0) {
			printf("<-1, cmd para error\n");
			return 0;
		}
	}

	if(strcmp(argv[1], "reset") == 0) {
		if(o2pf_config.loop) {
			if(!o2pf_config.o6) {
				mcu_vo2_n();
			}
		}

		o2pf_config.test = 0;
		o2pf_config.trig = 0;
		o2pf_config.open = 0;
		o2pf_config.loop = 0;
		o2pf_config.lamb = 0;
		valve_gpo_init(VALVE_EXT, 0);

		//average lamb
		o2pf_lmtest = o2pf_avg_l;
		o2pf_avg_l = o2pf_avg_t = 0;
		o2pf_avg_timer = 0;
	}
	else if(strcmp(argv[1], "open") == 0) {
		if(o2pf_config.test) {
			printf("<-2, refused dueto test is busy\n");
			return 0;
		}

		int vpw = (int)(v * 1000);
		o2pf_ext.timer = time_get(vpw);
		o2pf_ext.on = 0;
		valve_gpo_init(VALVE_EXT, 0);

		o2pf_config.vpw = vpw;
		o2pf_config.open = 1;
		o2pf_config.test = 1;
		o2pf_config.trig = 1;

		//average lamb
		o2pf_avg_l = o2pf_avg_t = 0;
		o2pf_avg_timer = time_get(0);
	}
	else if(strcmp(argv[1], "loop") == 0) {
		if(o2pf_config.test) {
			printf("<-2, refused dueto test is busy\n");
			return 0;
		}

		if(o2pf_config.o6) {
			printf("<-3, test is not supported\n");
			return 0;
		}

		mcu_vo2_y();
		o2pf_ext.timer = time_get(EXT_LOOP_DITHER_MS);
		o2pf_ext.on = 0;
		valve_gpo_init(VALVE_EXT, 0);

		o2pf_config.vth = v;
		o2pf_config.loop = 1;
		o2pf_config.test = 1;
		o2pf_config.trig = 1;

		//average lamb
		o2pf_avg_l = o2pf_avg_t = 0;
		o2pf_avg_timer = time_get(0);
	}
	else if(strcmp(argv[1], "lamb") == 0) {
		if(o2pf_config.test) {
			printf("<-2, refused dueto test is busy\n");
			return 0;
		}

		int vpw = (int)(v * 1000);
		o2pf_ext.timer = time_get(vpw);
		o2pf_ext.on = 0;
		o2pf_ext.df = vdf;
		valve_pwm_init(VALVE_EXT, vdf, 0);

		o2pf_config.vpw = vpw;
		o2pf_config.vdf = vdf;
		o2pf_config.lamb = 1;
		o2pf_config.test = 1;
		o2pf_config.trig = 1;
	}
	else {
		printf("<-4, sub cmd not supported\n");
		return 0;
	}

	printf("<+0, OK\n");
	return 0;
}

cmd_t cmd_test = {"test", cmd_test_func, "o2pf test cmds"};
DECLARE_SHELL_CMD(cmd_test)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
	};

	int ecode = 0;
	if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, ULICAR Technology,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, OK\n");
		NVIC_SystemReset();
	}
	return 0;
}
