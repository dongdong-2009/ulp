/*
 * 	miaofng@2010 initial version
 */
#ifndef __KEY_H_
#define __KEY_H_

#include "config.h"

//virtual key code table
enum {
	KEY_NONE,
	KEY_RESET,
	KEY_ENTER,
	KEY_MINUS,
	KEY_UP = 0x10, /*10*/
	KEY_DOWN = 0x11,
	KEY_MENU = 0x12,
	KEY_LEFT = 0x15,
	KEY_RIGHT = 0x16,
	KEY_SURF = 0x39,
	KEY_0 = 0x40,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9 = 0x49,
	KEY_POWER = 0x4c,
	KEY_MUTE = 0x4d,
	KEY_OSD = 0x4f,
	KEY_VOL_PLUS = 0x50,
	KEY_VOL_MINUS = 0x51,
	KEY_SMART_SOUND = 0x54,
	KEY_SMART_PICTURE = 0x55,
	KEY_CH_PLUS = 0x60,
	KEY_CH_MINUS = 0x61,
	KEY_STERO = 0x63,
	KEY_SURROUND = 0x64,
	KEY_SLEEP = 0x66,
	KEY_TIMER = 0x6f,
	KEY_AV = 0x78,
	KEY_CC = 0x7a,
	KEY_PLAY,
	KEY_FORWARD,
	KEY_BACKWARD,
	//special keycodes for encoder
	KEY_ENCODER_P, //+
	KEY_ENCODER_N, //-
	//special keycodes for touchscreen
	PDE_NONE,
	PDE_DN,
	PDE_UP,
	PDE_CLICK,
	PDE_HOLD,
	PDE_DRAG,
	KEY_DUMMY,
};

typedef union {
	struct {
		union {
			struct {
				short data : 6;
				short edata: 1;
				short system : 5;
			} rc5;
			short code; //key code
		};
		short flag_toggle : 1;
		short flag_nokey : 1;
	};
	int value;
} key_t;

enum {
	KEYBOARD_TYPE_LOCAL,
	KEYBOARD_TYPE_REMOTE,
	KEYBOARD_TYPE_ENCODER,
};

typedef struct {
	int (*init)(void);
	key_t (*getkey)(void);
} keyboard_t;

int key_Init(void);
int key_SetLocalKeymap(const int *keymap); //keymap must be ended with KEY_NONE
int key_GetKey(void);
int key_SetKeyScenario(int delay, int repeat); //unit: mS
int key_SetEntryAndGetDigit(void);

int keyboard_Add(const keyboard_t *kb, int kt);

#endif /*__KEY_H_*/
