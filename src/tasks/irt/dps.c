/*
*
*  miaofng@2014-6-4   initial version
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include <string.h>
#include "shell/shell.h"
#include "dps.h"
#include "bsp.h"

static float dps_lv;
static float dps_hs;
static float dps_is;
static float dps_vs;
static float dps_hv;

static int dps_flag_gain_auto;
static int dps_flag_enable;

static int dps_is_g;
static int dps_hv_g;

//VREF=2V5 R89 = 100K R88=47K R50=47K R81=1K
//VOUT = VREF * RATIO * DF = VMAX * DF
//=> DF = VOUT / VMAX
static int dps_lv_set(float v)
{
	#define lv_vmax (2.5 * 47/(100+47) * (47+1) / 1) //38v
	int pwm = (int) (v * (1024/ lv_vmax));
	pwm = (pwm > 1023) ? 1023 : pwm;
	lv_pwm_set(pwm);
	dps_lv = v;
	return 0;
}

//R76=20K R91=0.15 R90=4.7K GAIN = 8
//VADC = VOUT * RATIO * 8 => VOUT = VADC / 8 / RATIO
static float dps_lv_get(void)
{
	float vadc = lv_adc_get();
	vadc *= 2.5 / 65536;
	float vout = vadc / (8 * (0.15 / (20 + 0.15 + 4.7)));
	return vout;
}

/*GS0 INA225 GAIN, GS1 RELAY */
#define IS_GSR50 (1 << 1)
#define IS_GSR1R 0
#define IS_GS100 (1 << 0)
#define IS_GS025 0

#define IS_RSH  1.000 //ohm
#define IS_RSL  0.050 //ohm
#define IS_GH   100 //INA225 GAIN
#define IS_GL   25

/*NOTE:  2.5V*0.95 = 2.375*/
#define IS_RATIO_MAX 0.90

static int dps_is_set(float amp)
{
	float v; int pwm;
	//AUTO RANGE ADJUST
	if(dps_flag_gain_auto & (1 << DPS_IS)) {
		if(v > 0.5*IS_RATIO_MAX) dps_is_g = IS_GSR50 | IS_GS025;
		else if(v > 0.1*IS_RATIO_MAX) dps_is_g = IS_GSR50 | IS_GS100;
		else if(v > 0.025*IS_RATIO_MAX) dps_is_g = IS_GSR1R | IS_GS025;
		else dps_is_g = IS_GSR1R | IS_GS100;
	}

	dps_gain(DPS_IS, dps_is_g, 1);
	switch(dps_is_g) {
	case 0: //1R GAIN = 25
		v = amp * (IS_RSH * IS_GL);
		break;
	case 1: //1R GAIN = 100
		v = amp * (IS_RSH * IS_GH);
		break;
	case 2: //50mR, GAIN = 25
		v = amp * (IS_RSL * IS_GL);
		break;
	case 3:	//50mR, GAIN = 100
	default:
		v = amp * (IS_RSL * IS_GH);
		break;
	}

	pwm = (int) (v * (1024 / 2.5));
	pwm = (pwm > 1023) ? 1023 : pwm;
	is_pwm_set(pwm);
	return 0;
}

static int dps_hs_set(float v)
{
	return 0;
}

static int dps_vs_set(float v)
{
	return 0;
}

static int dps_hv_set(float v)
{
	return 0;
}

//6.8K + 1K
static float dps_hs_get(void)
{
	float vadc = hs_adc_get();
	vadc *= 2.5 / 65536;
	float vout = vadc * 7.8;
	return vout;
}

static float dps_vs_get(void)
{
	return 0;
}

static float dps_hv_get(void)
{
	return 0;
}


void dps_init(void)
{
}

void dps_update(void)
{
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
		bsp_gpio_set(LV_EN, enable);
		break;
	case DPS_HS:
		bsp_gpio_set(HS_EN, enable);
		break;
	case DPS_IS:
		break;
	case DPS_VS:
		break;
	case DPS_HV:
		bsp_gpio_set(HV_EN, enable);
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
	case DPS_LV:
		break;
	case DPS_HS:
		break;
	case DPS_IS:
		if(execute) {
			dps_is_g = gain;
			bsp_gpio_set(IS_GS0, gain & 0x01);
			bsp_gpio_set(IS_GS1, gain & 0x02);
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
	case DPS_VS:
		break;
	case DPS_HV:
		if(execute) {
			dps_hv_g = gain;
			bsp_gpio_set(HV_FS, gain & 0x01);
			bsp_gpio_set(HV_VS, gain & 0x02);
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
		break;
	default:
		ecode = IRT_E_OP_REFUSED;
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
	case DPS_P:
		break;
	case DPS_E:
		ecode = dps_enable(dps, iv);
		break;
	case DPS_G:
		ecode = dps_gain(dps, iv, 0);
		break;
	default:
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
	default:
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
					enable = 1;
					ecode = dps_config(dps, DPS_E, &enable);
				}
				else if(!strcmp(argv[2], "OFF")) {
					enable = 0;
					ecode = dps_config(dps, DPS_E, &enable);
				}
				else {
					int gain = (argc > 3) ? atof(argv[3]) : -1; //auto range
					float v = atof(argv[2]);

					ecode = dps_config(dps, DPS_G, &gain);
					if(ecode == 0) {
						ecode = dps_config(dps, DPS_V, &v);
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

