/*
 *	miaofng@2010 initial version
 */

#include "key.h"
#include "ulp/driver.h"
#include "encoder.h"
#include "ulp_time.h"
#include <stdio.h>

/*static variable*/
static key_t key; //this for up designer
static int key_v_save;

static int key_init(void)
{
	/*init static variables*/
	key.value = 0;
	key.flag_nokey = 1;
	key_v_save = 0;
	return encoder_Init();
}

static void key_update(void)
{
	int value = encoder_GetValue();
	int speed = encoder_GetSpeed();

	key.value = 0;
	if(value == key_v_save && speed == 0) {
		key.flag_nokey = 1;
		return;
	}

	key.code = (value > key_v_save || speed > 0) ? KEY_ENCODER_P : KEY_ENCODER_N;
	key_v_save = value;
}

static key_t key_getkey(void)
{
	key_update();
	return key;
}

static const keyboard_t key_encoder = {
	.init = key_init,
	.getkey = key_getkey,
};

static void key_reg_encoder(void)
{
	keyboard_Add(&key_encoder, KEYBOARD_TYPE_ENCODER);
}
driver_init(key_reg_encoder);
