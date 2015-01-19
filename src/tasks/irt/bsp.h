/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __BSP_H__
#define __BSP_H__

#include "config.h"

//#define IRC_BOARD_V2_1 1

void board_init(void);
void board_reset(void);
void trig_set(int high);
int trig_get(void); //vmcomp pulsed
void le_set(int high);
int le_get(void);
void rly_set(int mode);
void oe_set(int high);

#include "pwm.h"
#define lv_pwm_set pwm41.set
#define is_pwm_set pwm42.set
#define vs_pwm_set pwm43.set
#define hv_pwm_set pwm44.set

#ifdef IRC_BOARD_V2_1
	#define lv_adc_get adc_pc0_get
	#define hs_adc_get adc_pc2_get
	#define hv_adc_get adc_pc3_get
	#define vs_adc_get adc_pc1_get
#else
	#define lv_adc_get adc_pc3_get
	#define hs_adc_get adc_pc1_get
	#define hv_adc_get adc_pc0_get
	#define vs_adc_get adc_pc2_get
#endif

int adc_pc0_get(void);
int adc_pc1_get(void);
int adc_pc2_get(void);
int adc_pc3_get(void);
int adc_nul_get(void);

enum {
	IS_GS0,
	IS_GS1,
	LV_EN,
	HS_VS,
	HS_EN,
	VS_EN,
	HV_EN,
	HV_VS,

	//reserved pins
	HV_FS = VS_EN,
	IRSTART = HV_EN,
	IRTEST = HV_VS,
};

void bsp_gpio_set(int pin, int high);

#endif
