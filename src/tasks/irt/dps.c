/*
*
*  miaofng@2014-6-4   initial version
*  miaofng@2016-6-22  add dps lv board v2.0 support
*  DPS_LV:
*    pwm turn on = 1.5mS (note: vout at least = 1.225v)
*    pwm turn off = ... (determined by cout & load)
*    buck on = 19uS@2A, Surge = 192uS@3A(out is shorten)
*    buck off = 400uS(2A -> 0A)
*    so: buck on/off method is adopted!!!
*  DPS_IS:
*    pwm on = pwm off = 3mS(47K||10NF)
*    bank0: IS_GS0 = 0&IS_GS1 = 1, 0.12mA-122.5mA (1.225v/10ohm)
*    bank1: IS_GS0 = 1&IS_GS1 = 0, 2.04mA-2.085A (1.225V / (4.7ohm / 4) * 2)
*  miaofng@2016-7-23  add dps hv board v3.1 support
*  - static pwm hv configuration, >10mA@0-1KV drive capability
*  - dynamic vs up/down control and over-current protection(about 8mA)
*  - polar config & fast discharge (1kV->1A@1kohm)
*  miaofng@2016-8-18  add dps hv board v4.2 support
*  - add dps_hv(not vs) fast discharge support!
*  miaofng@2016-8-26  add dps lv board v2.1 support
*  - add dps is fast start/stop
*  miaofng@2016-8-29 unify dps_set() regular
*  - dps_hv_set() <=0 turn off hv_en, >0 turn on hv_en and keep vs_en
*  - dps_lv_set() <=0 turn off lv_en, >0 keep lv_en
*  - dps_is_set() <=0 turn off is_en, >0 keep is_en
*  miaofng@2017-10-09 add dps hv 2826 support
*  - add hv bank switch -
*  - hv_en auto enable (dueto lack of ctrl pin)
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include <string.h>
#include "shell/shell.h"
#include "dps.h"
#include "bsp.h"
#include <math.h>
#include "common/debounce.h"
#include "vm.h"

#define DPS_HV_AC 0 /*AUTO SWITCH HV POLAR*/
#define DPS_LV_2754 1 /*XL4015*/
#define DPS_HV_2826 1 //add hv bank, hv auto enable

#if DPS_LV_2754 == 1
#define IS_GS IS_GS0
#define IS_EN IS_GS1
#endif

#if DPS_HV_2826 == 1
#define CFG_DPS_HV_GAIN 1
#define CFG_DPS_HV_EN 0
#else
#define CFG_DPS_HV_GAIN 0
#define CFG_DPS_HV_EN 1
#endif

static float dps_lv;
static float dps_hv;
static float dps_hs;

static int dps_flag_gain_auto;
static int dps_flag_enable;

static int dps_is_g;
static int dps_hv_g;
static char dps_hv_polar = 0;

static struct debounce_s dps_mon_lv;
static struct debounce_s dps_mon_hs;
static struct debounce_s dps_mon_hv;

#define VREF_ADC 2.500

#if DPS_LV_2754 == 1
/*
vout = 1.25v + is * 10Kohm = 1.25v + vset / 0.5K * 10K
=> vset = ( vout - 1.25v ) / 20
*/
static int dps_lv_set(float v)
{
	int lv_enabled = dps_flag_enable & (1 << DPS_LV);
	if(lv_enabled) {
		dps_enable(DPS_LV, 0); //FAST DISCHARGE
		vm_wait(3); //1.4mS in the scope
	}

	dps_lv = v;
	if(v > 1.5) {
		dps_enable(DPS_LV, lv_enabled);

		float vset = (v - 1.25) / 20;
		int pwm = (int) (vset * 1024 / 1.225);
		pwm = (pwm > 1023) ? 1023 : pwm;
		lv_pwm_set(pwm);
		vm_wait(3); //2mS in the scope
	}
	return 0;
}

/*
vfb = vout * 3K / (27K + 3K)
vout = vfb * 10
*/
static float dps_lv_get(void)
{
	float vfb = lv_adc_get();
	vfb *= 2.5 / 65536;
	float vout = vfb * 10;
	return vout;
}

int dps_lv_start(int ms)
{
	if(dps_flag_enable & (1 << DPS_LV)) {
		//already started
	}
	else {
		if(dps_lv > 1.5) {
			dps_enable(DPS_LV, 1); //19uS@2A
			vm_wait(ms);
		}
	}
	return 0;
}

int dps_lv_stop(void)
{
	dps_enable(DPS_LV, 0); //400uS@2A
	return 0;
}
#endif

int dps_is_start(void)
{
	dps_enable(DPS_IS, 1);
	vm_wait(1);
	return 0;
}

int dps_is_stop(void)
{
	dps_enable(DPS_IS, 1);
	vm_wait(1);
	return 0;
}

#if DPS_LV_2754 == 1
//bank_LO: IS_GS = 0, 0.245mA-24.5mA (1.225v/50ohm)
//bank_HI: IS_GS = 1, 24.50mA-2450mA (1.225V/0.5ohm)
#define IS_BANK_LO 0x00 //0.2-100mA
#define IS_BANK_HI 0x01 //100-2000mA
static int dps_is_set(float amp)
{
	float v; int pwm;

	//AUTO RANGE ADJUST
	if(dps_flag_gain_auto & (1 << DPS_IS)) {
		if(amp > 0.120) dps_is_g = IS_BANK_HI;
		else if(amp < 0.080) dps_is_g = IS_BANK_LO;
	}

	dps_gain(DPS_IS, dps_is_g, 1);
	float Rsense = (dps_is_g == IS_BANK_HI) ? 0.5 : 10;
	v = amp * Rsense;

	pwm = (int) (v * (1024 / 1.225));
	pwm = (pwm > 1023) ? 1023 : pwm;
	is_pwm_set(pwm);

	//IS OFF?
	if(amp < 1.0e-6) {
		dps_enable(DPS_IS, 0);
		vm_wait(1);
	}
	return 0;
}
#endif

#if DPS_LV_2754 == 1
static int dps_hs_set(float v)
{
	//hs always 12v
	dps_hs = 12.0;
	return 0;
}
#endif

//6.8K + 1K
static float dps_hs_get(void)
{
	float vadc = hs_adc_get();
	vadc *= 2.5 / 65536;
	float vout = vadc * 7.8;
	return vout;
}

/*HV_EN is controled automaticly*/
#define HV_VREF_PWM 1.225
static int dps_hv_set(float v)
{
	v = (v <= 20.0) ? 0.0 : v; //too low ? turn off
	int hv_enabled = dps_flag_enable & (1 << DPS_HV);
	int vs_enabled = dps_flag_enable & (1 << DPS_VS);

	if(hv_enabled && v < dps_hv) {
		//fast discharge
		dps_enable(DPS_HV, 0);

		bsp_gpio_set(VS_UP, 1);
		bsp_gpio_set(VS_DN, 1);
		vm_wait(250); //123mS@(1kv,6mA)

		//resume vs settings
		dps_enable(DPS_VS, vs_enabled);
	}

	dps_hv = v;
	if(v > 0.0) {
		dps_enable(DPS_HV, 1);
	}
	{
		int ratio = 1000;
#if CFG_DPS_HV_GAIN == 1
		if(dps_flag_gain_auto & (1 << DPS_HV)) { //auto range adjust
			if(v > 99.0) dps_hv_g = 1;
			else dps_hv_g = 0;
		}
		dps_gain(DPS_HV, dps_hv_g, 1);
		ratio = (dps_hv_g) ? 1000 : 100;
#endif
		v /= ratio;

		int pwm = (int) (v * (1024/ HV_VREF_PWM));
		pwm = (pwm > 1023) ? 1023 : pwm;
		pwm = (pwm < 0) ? 0 : pwm;
		hv_pwm_set(pwm);
	}
	return 0;
}

static float dps_hv_get(void)
{
	float vadc = hv_adc_get();
	vadc *= VREF_ADC / 65536;
	int ratio = 1000;
#if CFG_DPS_HV_GAIN == 1
	ratio = (dps_hv_g) ? 1000 : 100;
#endif
	float vout = vadc * ratio;
	return vout;
}

static int dps_vs_set(float v)
{
	//vs only has switch on/off function
	return -IRT_E_OP_REFUSED;
}

static float dps_vs_get(void)
{
	float vadc = vs_adc_get();
	vadc *= VREF_ADC / 65536;

	int ratio = 1000;
#if CFG_DPS_HV_GAIN == 1
	ratio = (dps_hv_g) ? 1000 : 100;
#endif

	int g_adum = (27000 + 3000) / 3000; //27K, 3K
	float vout = vadc * ratio / g_adum;
	return vout;
}

void dps_init(void)
{
	dps_enable(DPS_LV, 0);
	dps_enable(DPS_HS, 0);
	dps_enable(DPS_HV, 0);
	dps_enable(DPS_VS, 0);

	//set default value
	dps_set(DPS_LV, 0.0);
	dps_set(DPS_HV, 0.0);
	dps_set(DPS_IS, 0.0);

	dps_hv_stop();
}

void dps_update(void)
{
	float lv, hs, hv, delta, ref;

#if 1
	//power hv decrease too slow :(
	//monitor hv output
	if(dps_flag_enable & (1 << DPS_HV)) {
		ref = dps_hv;
		hv = dps_hv_get();
		delta = hv - ref;
		delta = (delta > 0) ? delta : -delta;
		debounce(&dps_mon_hv, delta > 30.0);
		if(dps_mon_hv.on) {
			irc_error(-IRT_E_HV);
		}
	}
#endif

	//monitor lv output
	if(dps_flag_enable & (1 << DPS_LV)) {
		ref = dps_lv;
		lv = dps_lv_get();
		delta = lv - ref;
		delta = (delta > 0) ? delta : -delta;
		debounce(&dps_mon_lv, delta > 3.0);
		if(dps_mon_lv.on) {
			dps_set(DPS_LV, 0.0); //turn off
			irc_error(-IRT_E_LV);
		}
	}

	//monitor hs output
	if(dps_flag_enable & (1 << DPS_HS)) {
		ref = dps_hs;
		hs = dps_hs_get();
		delta = hs - ref;
		delta = (delta > 0) ? delta : -delta;
		debounce(&dps_mon_hs, delta > 2.0);
		if(dps_mon_hs.on) {
			dps_set(DPS_HS, 0.0); //turn off
			irc_error(-IRT_E_HS);
		}
	}
}

static void dps_mdelay(int ms)
{
	if(ms > 0) {
		time_t deadline = time_get(ms);
		while(time_left(deadline) > 0) {
			irc_update();
		}
	}
}

int dps_hv_start(void)
{
	//no needs to monitor vs now
	//instead, over-current is monitored from dmm
	dps_enable(DPS_VS, 1);
	return 0;
}

int dps_hv_stop(void)
{
	int ecode = - IRT_E_HV_DN;
	float vs;

	dps_enable(DPS_VS, 0);
	vm_wait(1);

	struct debounce_s vs_good;
	debounce_t_init(&vs_good, 5, 0); //ok more than 5mS
	time_t deadline = time_get(DPS_HVDN_MS);
	while(time_left(deadline) > 0) {
		irc_update();
		vs = dps_vs_get();
		debounce(&vs_good, vs < 49.0); //to avoid adum3190 vos issue (0.049v)
		if(vs_good.on) { //OK
			ecode = 0;
			break;
		}
	}

	if(ecode) {
		dps_enable(DPS_HV, 0);
		irc_error(ecode);
	}

	/*switch polar*/
	char polar = '+';
	#if DPS_HV_AC == 1
	if(dps_hv_polar == '+') polar = '-';
	else polar = '+';
	#else
	polar = (dps_hv_polar == '+') ? dps_hv_polar : '+';
	#endif

	if(polar != dps_hv_polar) {
		dps_hv_polar = polar;
		bsp_gpio_set(VS_POL, dps_hv_polar == '-');
		vm_wait(5); //relay response 5mS
	}

	return ecode;
}

int dps_enable(int dps, int enable)
{
	int ecode = 0;
	if(enable) {
		dps_flag_enable |= 1 << dps;
	}
	else {
		dps_flag_enable &= ~(1 << dps);
	}

	switch(dps) {
	case DPS_LV:
		debounce_t_init(&dps_mon_lv, 1000, 0); //max 1000mS, no error
		bsp_gpio_set(LV_EN, enable);
		break;
	case DPS_HS:
		debounce_t_init(&dps_mon_hs, 100, 0);
		bsp_gpio_set(HS_EN, enable);
		break;
	case DPS_VS:
		bsp_gpio_set(VS_UP, enable);
		bsp_gpio_set(VS_DN, !enable);
		break;
	case DPS_HV:
		debounce_t_init(&dps_mon_hv, 1000, 0);
#if CFG_DPS_HV_EN == 1
		bsp_gpio_set(HV_EN, enable);
#endif
		break;
	case DPS_IS:
		bsp_gpio_set(IS_EN, enable);
		break;
	default:
		ecode = IRT_E_OP_REFUSED;
	}

	return ecode;
}

int dps_gain(int dps, int gain, int execute)
{
	int ecode = 0;

	switch(dps) {
	case DPS_IS:
		if(execute) {
			dps_is_g = gain;
			bsp_gpio_set(IS_GS, gain);
			vm_wait(1); //t = q/i = cu / i = 100N * 5v / 8mA = 62uS
		}
		else {
			if(gain < 0) {
				dps_flag_gain_auto |=  1 << dps;
			}
			else {
				dps_flag_gain_auto &=  ~(1 << dps);
				dps_is_g = gain;
			}
		}
		break;
	case DPS_HV:
#if CFG_DPS_HV_GAIN == 1
		if(execute) {
			dps_hv_g = gain;
			bsp_gpio_set(HV_GS, gain & 0x01);
		}
		else {
			if(gain < 0) {
				dps_flag_gain_auto |=  1 << dps;
			}
			else {
				dps_flag_gain_auto &=  ~(1 << dps);
				dps_hv_g = gain;
			}
		}
#endif
		break;
	case DPS_VS:
	case DPS_LV:
	case DPS_HS:
		ecode = (gain == -1) ? 0 : -IRT_E_OP_REFUSED;
		break;
	default:
		ecode = -IRT_E_OP_REFUSED;
	}

	return ecode;
}

int dps_set(int dps, float v)
{
	int ecode = IRT_E_OP_REFUSED;

	switch(dps) {
	case DPS_LV:
		ecode = dps_lv_set(v);
		break;
	case DPS_HS:
		ecode = dps_hs_set(v);
		break;
	case DPS_IS:
		ecode = dps_is_set(v);
		break;
	case DPS_VS:
		ecode = dps_vs_set(v);
		break;
	case DPS_HV:
		ecode = dps_hv_set(v);
		break;
	default:
		ecode = -IRT_E_OP_REFUSED;
	}

	return ecode;
}

int dps_config(int dps, int key, void *p)
{
	int ecode = IRT_E_OP_REFUSED;
	int iv = *(int *) p;
	float fv = *(float *) p;;

	switch(key) {
	case DPS_V:
		ecode = dps_set(dps, fv);
		break;
	case DPS_E:
		ecode = dps_enable(dps, iv);
		break;
	case DPS_G:
		ecode = dps_gain(dps, iv, 0);
		break;
	case DPS_P:
	default:
		ecode = -IRT_E_OP_REFUSED;
	}

	return ecode;
}

float dps_sense(int dps)
{
	float v = -1;
	switch(dps) {
	case DPS_LV:
		v = dps_lv_get();
		break;
	case DPS_HS:
		v = dps_hs_get();
		break;
	case DPS_VS:
		v = dps_vs_get();
		break;
	case DPS_HV:
		v = dps_hv_get();
		break;
	default:;
	}

	return v;
}

static int name_2_dps(const char *name)
{
	const char *name_list[] = {"LV", "IS", "HS", "VS", "HV"};
	const int dps_list[] = {DPS_LV, DPS_IS, DPS_HS, DPS_VS, DPS_HV};

	int dps = -1;
	for(int i = 0; i < sizeof(dps_list) / sizeof(int); i ++) {
		if(!strcmp(name, name_list[i])) {
			dps = dps_list[i];
			break;
		}
	}
	return dps;
}

static int cmd_power_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"POWER ADC		dynamic display 4 way ain voltage\n"
		"POWER LV 5 		set LV=5v\n"
		"POWER LV ON/OFF	turn on/off LV\n"
		"POWER LV		sense LV output voltage\n"
		"POWER IS 0.4		set IS=0.4A, auto range\n"
		"POWER IS 0.4 0		set IS=0.4A, forced range = 0\n"
		"POWER HV UPDN		hv_start+hv_stop\n"
	};

	int ecode = -IRT_E_CMD_FORMAT;
	if((argc > 1) && !strcmp(argv[1], "ADC")) {
		float lv = lv_adc_get();
		float vs = vs_adc_get();
		float hs = hs_adc_get();
		float hv = hv_adc_get();

		lv = lv * 2.5 / 65536;
		vs = vs * 2.5 / 65536;
		hs = hs * 2.5 / 65536;
		hv = hv * 2.5 / 65536;

		printf("\rLV: %.3fv, VS: %.3fv, HS: %.3fv, HV: %.3fv", lv, vs, hs, hv);
		return 0;
	}
	else if((argc > 1) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}
	else if(argc > 1) {
		ecode = - IRT_E_CMD_PARA;
		int dps = name_2_dps(argv[1]);
		if(dps >= 0) {
			if(argc == 2) {
				float f = dps_sense(dps);
				printf("<%.3f\n\r", f);
				return 0;
			}
			else {
				int enable;
				if(!strcmp(argv[2], "ON")) {
					ecode = -IRT_E_OP_REFUSED_DUETO_ESYS;
					if(!irc_error_get()) {
						enable = 1;
						ecode = dps_config(dps, DPS_E, &enable);
					}
				}
				else if(!strcmp(argv[2], "OFF")) {
					enable = 0;
					ecode = dps_config(dps, DPS_E, &enable);
				}
				else if(!strcmp(argv[2], "UPDN")) {
					ecode = dps_hv_start();
					if(!ecode) {
						vm_wait(10);
						ecode = dps_hv_stop();
						if(!ecode) {
							ecode = dps_hv_start();
							if(!ecode) {
								vm_wait(10);
								ecode = dps_hv_stop();
								if(!ecode) {
									ecode = dps_hv_start();
									if(!ecode) {
										vm_wait(10);
										ecode = dps_hv_stop();
									}
								}
							}
						}
					}
				}
				else {
					ecode = -IRT_E_OP_REFUSED_DUETO_ESYS;
					if(!irc_error_get()) {
						int gain = (argc > 3) ? atoi(argv[3]) : -1; //auto range
						float v = atof(argv[2]);

						ecode = dps_config(dps, DPS_G, &gain);
						if(ecode == 0) {
							ecode = dps_config(dps, DPS_V, &v);
						}
					}
				}
			}
		}
	}

	irc_error_print(ecode);
	return 0;
}
const cmd_t cmd_power = {"POWER", cmd_power_func, "power board ctrl"};
DECLARE_SHELL_CMD(cmd_power)

