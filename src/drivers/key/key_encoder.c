/*
 *	miaofng@2010 initial version
 */

#include "key.h"
#include "driver.h"
#include "encoder.h"
#include "time.h"
#include <stdio.h>

/*static variable*/
static key_t key; //this for up designer
static char key_n_left; //how many digits waits in queue for uplevel
static int key_v_save;

int key_init(void)
{
	/*init static variables*/
	key.value = 0;
	key.flag_nokey = 1;
	key_n_left = -1;
	key_v_save = 0;
	return encoder_Init();
}

static void key_update(void)
{
	int value, n, flag = 0;
	char digits[16];

	if(key_n_left < 0) { //idle
		value = encoder_GetValue();
		if(value == key_v_save)
			return;
		
		key_v_save = value;
		flag = 1;
	}
	
	n = snprintf(digits, 16, "%d", value);
	key_n_left = flag ? n : key_n_left;
	n -= key_n_left;

	key.value = 0;
	if(digits[n] == '-') {
		key.code = KEY_MINUS;
	}
	else {
		key.code = digits[n] - '0' + KEY_0;
	}
	key.flag_nokey = (key_n_left == 0) ? 1 : 0;
	key_n_left -- ;
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

static void key_reg(void)
{
	keyboard_Add(&key_encoder, KEYBOARD_TYPE_ENCODER);
}
driver_init(key_reg);
