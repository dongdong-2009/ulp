/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/system.h"
#include "motor/motor.h"
#include "stm32f10x.h"
#include "normalize.h"
#include "vsm.h"

static int cmd_debug_func(int argc, char *argv[])
{
	int angle;
	short theta, sin, cos, atan;

	for(angle = 0; angle < (1<<17); angle += 100)
	{
		theta = (short) angle;
		mtri(theta, &sin, &cos);
		atan = matan(sin, cos);

		printf("angle %05d sin %05d cos %05d atan %05d\n", theta, sin, cos, atan);
	}

	return 0;
}
cmd_t cmd_debug = {"debug", cmd_debug_func, "for self-defined debug purpose"};
DECLARE_SHELL_CMD(cmd_debug)

/*motor algo debug*/
static int cmd_dalgo_counter;
static int cmd_dalgo_func(int argc, char *argv[])
{
	short angle, speed;
	short sin, cos;

	if(argc) {
		/*first time*/
		cmd_dalgo_counter = 0;
		motor_SetSpeed(100);
	}

	/*call motor_isr*/
	motor_isr();
	angle = smo_GetAngle();
	speed = smo_GetSpeed();
	speed = (short) _SPEED(speed);
	mtri(angle, &sin, &cos);

	printf("%04d ", cmd_dalgo_counter);
	printf("angle %d ",  angle);
	printf("speed %d ", speed);
	printf("sin %d ", sin);
	printf("cos %d ",  cos);
	printf("Iab %d %d ",  Iab.a,  Iab.b);
	printf("I %d %d ",  I.alpha,  I.beta);
	printf("Idq %d %d ",  Idq.d,  Idq.q);
	printf("Vdq %d %d ",  Vdq.d,  Vdq.q);
	printf("V %d %d ",  V.alpha,  V.beta);
	printf("\n");

	cmd_dalgo_counter ++;
	return 1;
}
cmd_t cmd_dalgo = {"dalgo", cmd_dalgo_func, "motor algo debug"};
DECLARE_SHELL_CMD(cmd_dalgo)

#define CMD_SVPWM_TS	5 /*unit; mS*/
static time_t cmd_svpwm_timer; /*update timer*/
static int cmd_svpwm_index; /*0,1,2,3,4....*/
static vector_t cmd_svpwm_vdq; /*setting voltage*/
static int cmd_svpwm_angle_inc; /*setting period*/
static short cmd_svpwm_angle;
static int cmd_svpwm_func(int argc, char *argv[])
{
	int v1, v2, i1, i2, hz;
	vector_t v, iab, i;
	char usage[] = { \
		" usage:\n" \
		" svpwm vd vq hz\n" \
		" vd/vq, d/q axis voltage, unit mV\n" \
		" hz, input modulation signal freq to vsm driver, pls do not exceed 10Hz\n" \
	};

	/*first loop*/
	if(argc) {
		if(argc != 4) {
			printf("%s", usage);
			return 0;
		}

		/*save voltage*/
		v1 = atoi(argv[1]);
		v2 = atoi(argv[2]);
		cmd_svpwm_vdq.d = (short)NOR_VOL(v1);
		cmd_svpwm_vdq.q = (short)NOR_VOL(v2);

		/*save freq*/
		hz = atoi(argv[3]);
		cmd_svpwm_angle_inc = (hz << 16) * CMD_SVPWM_TS / 1000;

		cmd_svpwm_timer = time_get(CMD_SVPWM_TS);
		cmd_svpwm_index = -1;
		cmd_svpwm_angle = 0;

		vsm_Start();
		ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
		return 1;
	}

	if(time_left(cmd_svpwm_timer) > 0)
		return 1;

	/*deadloop start here...*/
	cmd_svpwm_timer = time_get(CMD_SVPWM_TS);
	cmd_svpwm_index ++;

	/*invert transform*/
	ipark(&cmd_svpwm_vdq, &v, cmd_svpwm_angle);
	v1 = _VOL(v.alpha);
	v2 = _VOL(v.beta);
	vsm_SetVoltage(v.alpha, v.beta);

	/*wait for adc result*/
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC));
	vsm_GetCurrent(&iab.a, &iab.b);
	i1 = _AMP(iab.a);
	i2 = _AMP(iab.b);
	clarke(&iab, &i);
	ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);

	/*calc current angle*/
	cmd_svpwm_angle += cmd_svpwm_angle_inc;

	/*display*/
	printf("%05d: phi %d V %d %d", cmd_svpwm_index, cmd_svpwm_angle, v1, v2);
	printf(" I %d %d", i.alpha, i.beta);
	printf(" Iab %d %d", i1, i2);
	printf("\n");
	return 1;
}
cmd_t cmd_svpwm = {"svpwm", cmd_svpwm_func, "svpwm output test"};
DECLARE_SHELL_CMD(cmd_svpwm)
