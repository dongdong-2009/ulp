/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add content for PDI_SDM10
 */
#include "config.h"
#include "ulp_time.h"
#include "drv.h"
#include "stm32f10x.h"
#include "sys/task.h"
#include "led.h"

#ifdef CONFIG_PDI_SDM10
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define Beep_on				(1<<2)
#define start_botton		(1<<3)
#define counter_pass		(1<<4)
#define counter_fail		(1<<5)
#define target				(1<<7)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define IGN_on				(1<<13)
#define battary_on			(1<<15)
#endif

#ifdef CONFIG_PDI_DM
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define target				(1<<2)
#define start_botton		(1<<3)
#define counter_pass		(1<<4)
#define counter_fail		(1<<5)
#define Beep_on				(1<<8)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define battary_on			(1<<13)
#define IGN_on				(1<<15)
#endif

#ifdef CONFIG_PDI_RC
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define target				(1<<7)
#define start_botton		(1<<3)
#define counter_pass		(1<<4)
#define counter_fail		(1<<5)
#define JAMA				(1<<2)
#define Beep_on				(1<<8)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define battary_on			(1<<13)
#define IGN_on				(1<<15)
#endif

#ifdef CONFIG_PDI_B515
#define start_botton		(1<<4)
#define counter_fail		(1<<5)
#define target				(1<<2)
#define counter_pass		(1<<3)
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define Beep_on				(1<<6)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define battary_on			(1<<13)
#define IGN_on				(1<<15)
#endif

#ifdef CONFIG_PDI_J04N
#define start_botton		(1<<3)
#define counter_fail		(1<<4)
#define target				(1<<7)
#define counter_pass		(1<<5)
#define LED_red_on			(1<<1)
#define LED_green_on		(1<<0)
#define JAMA				(1<<2)
#define Beep_on				(1<<8)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define battary_on			(1<<13)
#define IGN_on				(1<<15)
#endif

#ifdef CONFIG_PDI_RP
#define start_botton		(1<<0)
#define counter_fail		(1<<1)
#define target				(1<<2)
#define counter_pass		(1<<3)
#define LED_red_on			(1<<4)
#define LED_green_on		(1<<5)
#define Beep_on				(1<<6)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define battary_on			(1<<13)
#define IGN_on				(1<<15)
#endif

#ifdef CONFIG_PDI_SDM30
#define LED_red_on			(1<<0)
#define LED_green_on		(1<<1)
#define Beep_on				(1<<8)
#define start_botton		(1<<3)
#define counter_pass		(1<<5)
#define counter_fail		(1<<4)
#define target				(1<<2)
#define swcan_mode0			(1<<10)
#define swcan_mode1			(1<<11)
#define IGN_on				(1<<15)
#define battary_on			(1<<9)
#endif

//static time_t check_fail_beep;
int pdi_batt_on()
{
	GPIOE->ODR |= battary_on;
	return 0;
}

int pdi_batt_off()
{
	GPIOE->ODR &= ~battary_on;
	return 0;
}

int pdi_IGN_on()
{
	GPIOE->ODR |= IGN_on;
	return 0;
}

int pdi_IGN_off()
{
	GPIOE->ODR &= ~IGN_on;
	return 0;
}

int pdi_drv_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

#ifdef CONFIG_PDI_SDM10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_DM
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_RC
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_B515
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_J04N
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_RP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

#ifdef CONFIG_PDI_SDM30
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_8|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_9|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

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

int counter_pass_rise()
{
	GPIOE->ODR |= counter_pass;
	return 0;
}

int counter_pass_down()
{
	GPIOE->ODR &= ~counter_pass;
	return 0;
}

int counter_fail_rise()
{
	GPIOE->ODR |= counter_fail;
	return 0;
}

int counter_fail_down()
{
	GPIOE->ODR &= ~counter_fail;
	return 0;
}

int target_on()
{
	if((GPIOE->IDR & target) == 0)
		return 1;
	else
		return 0;
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

#ifdef CONFIG_PDI_RC
int JAMA_on()
{
	if((GPIOE->IDR & JAMA) == 0)		//如果有JAMA返回为0，否则返回为1
		return 0;
	else
		return 1;
}
#endif

#ifdef CONFIG_PDI_J04N
int JAMA_on()
{
	if((GPIOE->IDR & JAMA) == 0)		//如果有JAMA返回为0，否则返回为1
		return 0;
	else
		return 1;
}
#endif
