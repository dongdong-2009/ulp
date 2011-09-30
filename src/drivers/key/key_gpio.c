/*
 *	miaofng@2010 initial version
 */

#include "key.h"
#include "driver.h"
#include "time.h"

#define NOKEY 0xff
#define KEY_UPDATE_MS (10)
#define KEY_DETECT_CNT (5) /*detect time = KEY_DETECT_CNT * KEY_UPDATE_MS*/

/*static variable*/
static key_t key; //this for up designer
static char key_counter;
static time_t key_timer;

#if CONFIG_GPIOKEY_LAYOUT_GB == 1
#include "stm32f10x.h"

static int key_hwinit(void)
{
	/* key pin map:
		key0, S3	PC13
		key1, S2	PA0, note: pls connect jumper JP2 pin 1 and 2
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	return 0;
}

static short key_hwgetvalue(void)
{
	short value = 0;
	value |= (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0));
	value <<= 1;
	value |= (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13));

	if(value == 0)
		value = NOKEY;
	else
		value -= 1;

	return value;
}
#endif

#if CONFIG_GPIOKEY_LAYOUT_VVT == 1
#include "stm32f10x.h"

static int key_hwinit(void)
{
	/* key pin map:
		key0, S_ENCODER		PB11
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	return 0;
}

static short key_hwgetvalue(void)
{
	short value = 0;
	value |= (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11));

	if(value == 0)
		value = NOKEY;
	else
		value -= 1;

	return value;
}
#endif

static int key_init(void)
{
	/*init static variables*/
	key.value = 0;
	key.flag_nokey = 1;
	key_counter = 0;
	key_timer = time_get(KEY_UPDATE_MS);

	return key_hwinit();
}

static void key_update(void)
{
	short value;

	if(time_left(key_timer) > 0)
		return;
	key_timer = time_get(KEY_UPDATE_MS);

	//get key code from gpio level
	value = key_hwgetvalue();

	//update?
	if(value != key.code) {
		key_counter ++;
		if(key_counter >= KEY_DETECT_CNT) {
			key.value = 0;
			key.code = value;
			if(value == NOKEY) {
				key.flag_nokey = 1;
			}
			else {
				key.code = value;
			}
		}
	}
	else {
		key_counter = 0;
	}
}

static key_t key_getkey(void)
{
	key_update();
	return key;
}

static const keyboard_t key_gpio = {
	.init = key_init,
	.getkey = key_getkey,
};

static void key_reg_gpio(void)
{
	keyboard_Add(&key_gpio, KEYBOARD_TYPE_LOCAL);
}
driver_init(key_reg_gpio);
