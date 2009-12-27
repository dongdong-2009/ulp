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
	printf("angle %04x ", (unsigned short)angle);
	printf("speed %03d ", speed);
	printf("sin %04x ", (unsigned short)sin);
	printf("cos %04x ", (unsigned short)cos);
	printf("Iab %04x %04x ", (unsigned short)Iab.a, (unsigned short)Iab.b);
	printf("I %04x %04x ", (unsigned short)I.alpha, (unsigned short)I.beta);
	printf("Idq %04x %04x ", (unsigned short)Idq.d, (unsigned short)Idq.q);
	printf("Vdq %04x %04x ", (unsigned short)Vdq.d, (unsigned short)Vdq.q);
	printf("V %04x %04x ", (unsigned short)V.alpha, (unsigned short)V.beta);
	printf("Vab %04x %04x ", (unsigned short)Vab.a, (unsigned short)Vab.b);
	printf("\n");
	
	cmd_dalgo_counter ++;
	return 1;
}
cmd_t cmd_dalgo = {"dalgo", cmd_dalgo_func, "motor algo debug"};
