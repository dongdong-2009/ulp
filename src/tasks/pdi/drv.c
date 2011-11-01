/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add content for PDI_SDM10
 */
#include "config.h"
#include "ulp_time.h"
#include "drv.h"
#include "stm32f10x.h"
#include "sys/task.h"

#define Beep_on				(1<<2)
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define counter_pass		(1<<4)
#define counter_fail		(1<<5)
#define target				(1<<7)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define IGN_on				(1<<13)
#define battary_on			(1<<15)
#define start_botton		(1<<3)

//static time_t check_fail_beep;

int pdi_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			ulp_update();
		}
	} while(left > 0);

	return 0;
}

int power_on()
{
	GPIOE->ODR |= IGN_on;
	GPIOE->ODR |= battary_on;
	return 0;
}

int power_off()
{
	GPIOE->ODR &= ~IGN_on;
	GPIOE->ODR &= ~battary_on;
	return 0;
}

int pdi_drv_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	return 0;
}

int led_fail_on()
{
	GPIOE->ODR |= LED_red_on;
	return 0;
}

int led_fail_off()
{
	GPIOE->ODR &= ~LED_red_on;
	return 0;
}

int led_pass_on()
{

	GPIOE->ODR |= LED_green_on;
	return 0;
}

int led_pass_off()
{
	GPIOE->ODR &= ~LED_green_on;
	return 0;
}

int beep_on()
{
	GPIOE->ODR |= Beep_on;
	return 0;
}

int beep_off()
{
	GPIOE->ODR &= ~Beep_on;
	return 0;
}

int counter_pass_add()
{
	GPIOE->ODR |= counter_pass;
	pdi_mdelay(30);
	GPIOE->ODR &= ~counter_pass;
	return 0;
}

int counter_fail_add()
{
	GPIOE->ODR |= counter_fail;
	pdi_mdelay(30);
	GPIOE->ODR &= ~counter_fail;
	return 0;
}

int target_on()
{
	if((GPIOE->IDR & target) == 0)
		return 1;
	else return 0;
}

int pdi_swcan_mode()
{
	GPIOE->ODR |= swcan_mode0;
	GPIOE->ODR |= swcan_mode1;
	return 0;
}

int start_botton_on()
{
	GPIOE->ODR |= start_botton;
	return 0;
}

int start_botton_off()
{
	GPIOE->ODR &= ~start_botton;
	return 0;
}

int init_OK()
{
	GPIOE->ODR |= LED_red_on;
	GPIOE->ODR |= LED_green_on;
	GPIOE->ODR |= Beep_on;
	pdi_mdelay(200);
	GPIOE->ODR &= ~LED_red_on;
	GPIOE->ODR &= ~LED_green_on;
	GPIOE->ODR &= ~Beep_on;
	pdi_mdelay(100);
	for(int i = 0; i < 5; i++){
		GPIOE->ODR |= LED_red_on;
		GPIOE->ODR |= LED_green_on;
		pdi_mdelay(200);
		GPIOE->ODR &= ~LED_red_on;
		GPIOE->ODR &= ~LED_green_on;
		pdi_mdelay(100);
	}
	return 0;
}
