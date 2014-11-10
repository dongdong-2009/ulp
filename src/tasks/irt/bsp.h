/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __BSP_H__
#define __BSP_H__

#include "config.h"

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

int lv_adc_get(void);
int vs_adc_get(void);
int hs_adc_get(void);
int hv_adc_get(void);

enum {
	IS_GS0,
	IS_GS1,
	LV_EN,
	HS_VS,
	HS_EN,
	HV_FS,
	HV_EN,
	IRSTART,
	HV_VS,
	IRTEST,
};

void bsp_gpio_set(int pin, int high);

#endif
