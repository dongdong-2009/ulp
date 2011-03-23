/*
 * 	miaofng@2011 initial version
*		- common encoder driver
 */

#include "config.h"
#include "debug.h"
#include "time.h"
#include "encoder.h"

static short encoder_v_min;
static short encoder_v_max;
static short encoder_v;

//hw encoder status
static short encoder_v_save; //previous hw encoder value
static short encoder_speed_avg;
static time_t encoder_t_save; //previous turn time

int encoder_Init(void)
{
	encoder_v_min = 0;
	encoder_v_max = 100;
	encoder_v = 0;
	encoder_v_save = 0;
	encoder_speed_avg = 0;
	encoder_t_save = time_get(0);
	return encoder_hwInit();
}

int encoder_SetRange(int min, int max)
{
	encoder_v_min = (short) min;
	encoder_v_max = (short) max;
	return 0;
}

static void encoder_Update(void)
{
	int v, t, speed, ratio;
	short temp = encoder_hwGetValue();
	if(temp != encoder_v_save) {
		v = temp - encoder_v_save;
		t = - time_left(encoder_t_save);
		encoder_t_save = time_get(0);
		encoder_v_save = (short) temp;

		//speed-value regulation
		speed = (v * 1000) / t; // unit: pulse per second
		speed = (speed < 0) ? -speed : speed;
		speed = (speed + encoder_speed_avg) >> 1;
		encoder_speed_avg = (short) speed;
#ifdef CONFIG_ENCODER_CMD
		extern short encoder_speed;
		if(speed > 999) {
			//speed = speed + temp + t;
			speed = 1;
		}
		encoder_speed = (short) speed;
#endif
		ratio = speed;
		ratio *= (encoder_v_max - encoder_v_min) / 50; //normal encoder range:  10pulse * 5 turn = 50
		ratio /= (10 * 1000 / 200); //normal speed = 10 pulse / 20mS
		//ratio *= speed;
		ratio = (ratio < 1) ? 1 : ratio; //ratio always bigger than 1(takes effect only in fast situation)
		encoder_v += (short) (v * ratio);

		//val limitation
		encoder_v = (encoder_v > encoder_v_max) ? encoder_v_max : encoder_v;
		encoder_v = (encoder_v < encoder_v_min) ? encoder_v_min : encoder_v;
	}
	else {
		encoder_speed_avg /= 2;
	}
}

int encoder_GetValue(void)
{
	encoder_Update();
	return encoder_v;
}
