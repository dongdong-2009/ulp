/*
 * 	miaofng@2010 initial version
 */
#ifndef __KEY_H_
#define __KEY_H_

//virtual key code table
enum {
	KEY_NONE,
	KEY_RESET,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_ENTER,
	KEY_UP,
	KEY_DOWN,
};

typedef union {
	struct {
		union {
			struct {
				int data : 6;
				int system : 5;
			} rc5;
			short int code; //key code
		};
		int flag_toggle : 1;
		int flag_nokey : 1;
	};
	int value;
} key_t;

typedef struct {
	int (*init)(void);
	key_t (*getkey)(void);
} keyboard_t;

int key_Init(void);
int key_SetLocalKeymap(const int *keymap); //keymap must be ended with KEY_NONE
int key_GetKey(void);
int key_SetKeyScenario(int delay, int repeat); //unit: mS

#endif /*__KEY_H_*/
