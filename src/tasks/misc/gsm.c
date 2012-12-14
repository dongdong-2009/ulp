/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "led.h"
#include "key.h"
#include "stm32f10x.h"

/*led1-4 for key event display
when key1 is pressed, led1 will be turned off.
it keeps off state until the gsm message been sent.
it flashes the ecode in case of op fail

led5-8 is controlled by gsm message only
*/
const char led_pinmap[] = {0,6,7,10,11,12,13,14,15};
void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	//led gpio init, pb6~7, pb10~pb15
	GPIO_InitStructure.GPIO_Pin = 0xfcc0; //1111 1100 1100 0000
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*para: led = 1~8, on=0/1*/
#define led_set_status(led, on) do { \
	 GPIOB->BSRR = (1 << (led_pinmap[led] + ((on) ? 0 : 16))); \
} while(0)

/*gpio keyboard hardware driver*/
int key_hwinit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	//key init, s1=pb0, s2=pc13, s3=pb1, s4=pb5
	GPIO_InitStructure.GPIO_Pin = 0x0023; //0000 0000 0010 0011
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = 1 << 13;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	return 0;
}

/*return keycode: 1~4 or NOKEY*/
int key_hwgetvalue(void)
{
	int value = 0;
	if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0))
		value = 1;
	else if(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))
		value = 2;
	else if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1))
		value = 3;
	else if(!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5))
		value = 4;
	else
		value = NOKEY;

	return value;
}


void gsm_mdelay(int ms)
{
	time_t deadline = time_get(ms);
	while(time_left(deadline) > 0) {
		sys_update();
	}
}

void main(void)
{
	sys_init();
	led_init();
	key_Init();
	for(int i = 1; i < 8; i ++)
		led_set_status(i, 1);

	while(1) {
		int key= key_GetKey();
		if(key != 0) {
			led_set_status(key, 0);
			gsm_mdelay(1000);
			led_set_status(key, 1);
		}
		sys_update();
	}
}
