/*
 * 	miaofng@2010 initial version
*		- common keyboard driver
*		- delay&repeat key scenario is supported
 */

#include "config.h"
#include "stm32f10x.h"
#include "key.h"
#include "time.h"
#include <stddef.h>

static keyboard_t *key_local;
static keyboard_t *key_remote;

static const int *key_map; //for local keyboard usage
static int key_map_len; //nr of keys, exclude KEY_NONE

static key_t key_previous;
static time_t key_timer; //delay or repeat key timer
static int key_time_repeat;

int key_Init(void)
{
	key_local = NULL;
	key_remote = NULL;

	key_map = NULL;
	key_map_len = 0;

	key_previous.value = 0;
	key_previous.flag_nokey = 1;
	key_timer = 0;
	key_time_repeat = 0;

	//hardware keyboard driver init
	if(key_local != NULL)
		key_local->init();
	if(key_remote != NULL)
		key_remote->init();
	return 0;
}

int key_SetLocalKeymap(const int *keymap)
{
	int i = 0;
	while(*(keymap + i) != KEY_NONE)
		i ++;

	key_map_len = i;
	key_map = keymap;
	return 0;
}

int key_GetKey(void)
{
	key_t key;
	int idx;
	int pass = 0;

	//default to nokey
	key.value = 0;
	key.flag_nokey = 1;

	//fetch a new key from local keyboard
	if(key_local != NULL) {
		key = key_local->getkey();
		if(key_map != NULL && key.flag_nokey != 1) {
			idx = key.code;
			idx = (idx > key_map_len) ? key_map_len : idx;
			key.code = key_map[idx];
		}
	}

	//fetch a new key from remote control
	if(key_remote != NULL) {
		if(key.flag_nokey)
			key = key_remote->getkey();
	}

	//key scenario handling
	if(key.value == key_previous.value) { //held or repeated key
		if(key_timer != 0) {
			if(time_left(key_timer) < 0) {
				pass = 1;
				if(key_time_repeat > 0)
					key_timer = time_get(key_time_repeat);
				else
					key_timer = 0;
			}
		}
	}
	else { //first key
		pass = 1;
		key_previous = key;
		key_timer = 0;
		key_time_repeat = 0;
	}

	//return key.code
	return (pass == 1) ? key.code : KEY_NONE;
}

int key_SetKeyScenario(int delay, int repeat)
{
	key_timer = time_get(delay);
	key_time_repeat = repeat;
	return 0;
}
