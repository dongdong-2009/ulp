/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "motor.h"

static time_t debug_deadline;
static int debug_counter;

static int cmd_debug_func(int argc, char *argv[])
{
	if(!debug_counter) {
		/*first time run*/
		debug_counter = 10;
		debug_deadline = time_get(1000);
		led_flash(LED_GREEN);
		return 1;
	}
	
	led_update();
	if(time_left(debug_deadline) < 0) {
		debug_deadline = time_get(1000);
		debug_counter --;
		if(debug_counter > 0)
			printf("\rdebug_ms_counter = %02d ", debug_counter);
		else {
			printf("debug command is over!!\n");
			return 0;
		}
	}
	return 1;
}
cmd_t cmd_debug = {"debug", cmd_debug_func, "for self-defined debug purpose"};

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
	printf("Vab %d %d ",  Vab.a,  Vab.b);
	printf("\n");
	
	cmd_dalgo_counter ++;
	return 1;
}
cmd_t cmd_dalgo = {"dalgo", cmd_dalgo_func, "motor algo debug"};

#define CMD_SVPWM_TS	2 /*unit; mS*/
static time_t cmd_svpwm_timer; /*update timer*/
static int cmd_svpwm_index; /*0,1,2,3,4....*/
static vector_t cmd_svpwm_vdq; /*setting voltage*/
static int cmd_svpwm_angle_inc; /*setting period*/
static short cmd_svpwm_angle;
static int cmd_svpwm_func(int argc, char *argv[])
{
	int v1, v2, hz;
	vector_t v, vab;
	char usage[] = { \
		"usage:\n" \
		" svpwm vd vq hz\n" \
		" vd/vq, d/q axis voltage, unit mV\n" \
		" hz, input modulation signal freq to vsm driver, pls do not exceed 100Hz\n" \
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
		cmd_svpwm_angle_inc = hz * midShort * CMD_SVPWM_TS / 1000;
		
		cmd_svpwm_timer = CMD_SVPWM_TS;
		cmd_svpwm_index = -1;
		cmd_svpwm_angle = 0;
		
		vsm_Start();
		return 1;
	}
	
	if(time_left(cmd_svpwm_timer) > 0)
		return 1;
	
	/*deadloop start here...*/
	cmd_svpwm_timer = time_get(CMD_SVPWM_TS);
	cmd_svpwm_index ++;
	
	/*invert transform*/
	ipark(&cmd_svpwm_vdq, &v, cmd_svpwm_angle);
	iclarke(&v, &vab);
	v1 = _VOL(vab.a);
	v2 = _VOL(vab.b);
	vsm_SetVoltage(v1, v2);
	
	/*calc current angle*/
	cmd_svpwm_angle += cmd_svpwm_angle_inc;
	
	/*display*/
	printf("%05d: phi %d u %d v %d w %d\n", cmd_svpwm_index, cmd_svpwm_angle, v1, v2, (-v1-v2));
	return 1;
}
cmd_t cmd_svpwm = {"svpwm", cmd_svpwm_func, "svpwm output test"};

