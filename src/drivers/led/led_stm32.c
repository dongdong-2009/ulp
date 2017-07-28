/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "led.h"
#include "stm32f10x.h"

void led_hwInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1) || (CONFIG_TASK_VVT == 1) || (CONFIG_TASK_SSS == 1)
	/* Enable GPIOA,GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	/* Configure PC.10 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure PC.12 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#elif CONFIG_CAN_BPMON == 1
	//PC4->GREEN, PC5->RED
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#elif CONFIG_MISC_VHD == 1
	//PC6->red, PC7->yellow, PC8->green
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	/* Configure PC.6, PC.7, PC.8 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#elif (CONFIG_TASK_PDI == 1) || (CONFIG_IRT_IRC == 1) || (CONFIG_MISC_O2PT == 1) || (CONFIG_MISC_O2PTV2 == 1)
	//PE0->red, PE1->green
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#elif (CONFIG_IRT_PROBE == 1)
	//PB10->red, PB11->green
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#elif CONFIG_MISC_ICT == 1
	//PC8 led green
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#elif CONFIG_BOARD_HY_SMART == 1
	//PB1 led green
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#elif CONFIG_MISC_MATRIX == 1
	/*PA0 RLED, PA1 GLED*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#elif (CONFIG_OID_HWV2 == 1) || (CONFIG_YBS_MON == 1) || (CONFIG_IRT_MXC5324 == 1) || \
	(CONFIG_IRT_UMX == 1) || (CONFIG_MISC_HUBCTRL == 1)  || (CONFIG_MISC_VWPLC == 1) || (CONFIG_MISC_VWHOST == 1)
	/*PC0 RLED, PC1 GLED*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#else
	/*led_r PE14, led_g PE15*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif
}

void led_hwSetStatus(led_t led, led_status_t status)
{
	BitAction ba;
	switch(status) {
		case LED_OFF:
			ba = Bit_RESET;
			break;
		case LED_ON:
			ba = Bit_SET;
			break;
		default:
			return;
	}

	switch(led) {
		case LED_GREEN:
#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1) || (CONFIG_TASK_VVT == 1) || (CONFIG_TASK_SSS == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_12, ba);
#elif CONFIG_CAN_BPMON == 1
			GPIO_WriteBit(GPIOC, GPIO_Pin_5, ba);
#elif CONFIG_MISC_VHD == 1
			GPIO_WriteBit(GPIOC, GPIO_Pin_8, ba);
#elif (CONFIG_TASK_PDI == 1) || (CONFIG_IRT_IRC == 1) || (CONFIG_MISC_O2PT == 1) || (CONFIG_MISC_O2PTV2 == 1)
			#if CONFIG_PDI_RSU4 == 1
			GPIO_WriteBit(GPIOE, GPIO_Pin_15, ba);
			#else
			GPIO_WriteBit(GPIOE, GPIO_Pin_1, ba);
			#endif
#elif CONFIG_IRT_PROBE == 1
			GPIO_WriteBit(GPIOB, GPIO_Pin_11, ba);
#elif CONFIG_MISC_ICT == 1
			GPIO_WriteBit(GPIOC, GPIO_Pin_8, ba);
#elif CONFIG_BOARD_HY_SMART == 1
			GPIO_WriteBit(GPIOB, GPIO_Pin_1, ba);
#elif CONFIG_MISC_MATRIX == 1
			GPIO_WriteBit(GPIOA, GPIO_Pin_1, ba);
#elif (CONFIG_MISC_COMPASS == 1) || (CONFIG_OID_HWV2 == 1) || (CONFIG_YBS_MON == 1) || (CONFIG_IRT_MXC5324 == 1)  || \
	(CONFIG_IRT_UMX == 1) || (CONFIG_MISC_HUBCTRL == 1) || (CONFIG_MISC_VWPLC == 1) || (CONFIG_MISC_VWHOST == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_1, ba);
#else
			GPIO_WriteBit(GPIOE, GPIO_Pin_15, ba);
#endif
			break;
		case LED_RED:
#if (CONFIG_TASK_MOTOR == 1) || (CONFIG_TASK_STEPMOTOR == 1) || (CONFIG_TASK_VVT == 1) || (CONFIG_TASK_SSS == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_10, ba);
#elif CONFIG_CAN_BPMON == 1
			GPIO_WriteBit(GPIOC, GPIO_Pin_4, ba);
#elif CONFIG_MISC_VHD == 1
			GPIO_WriteBit(GPIOC, GPIO_Pin_6, ba);
#elif (CONFIG_TASK_PDI == 1) || (CONFIG_IRT_IRC == 1) || (CONFIG_MISC_O2PT == 1) || (CONFIG_MISC_O2PTV2 == 1)
			#if CONFIG_PDI_RSU4 == 1
			GPIO_WriteBit(GPIOE, GPIO_Pin_14, ba);
			#else
			GPIO_WriteBit(GPIOE, GPIO_Pin_0, ba);
			#endif
#elif CONFIG_IRT_PROBE == 1
			GPIO_WriteBit(GPIOB, GPIO_Pin_10, ba);
#elif CONFIG_MISC_MATRIX == 1
			GPIO_WriteBit(GPIOA, GPIO_Pin_0, ba);
#elif (CONFIG_MISC_COMPASS == 1) || (CONFIG_OID_HWV2 == 1) || (CONFIG_YBS_MON == 1) || (CONFIG_IRT_MXC5324 == 1)  || \
	(CONFIG_IRT_UMX == 1) || (CONFIG_MISC_HUBCTRL == 1) || (CONFIG_MISC_VWPLC == 1) || (CONFIG_MISC_VWHOST == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_0, ba);
#else
			GPIO_WriteBit(GPIOE, GPIO_Pin_14, ba);
#endif
			break;
		case LED_YELLOW:
#if (CONFIG_OID_HWV2 == 1) || (CONFIG_YBS_MON == 1) || (CONFIG_IRT_MXC5324 == 1)  || (CONFIG_IRT_UMX == 1) || (CONFIG_MISC_HUBCTRL == 1)
			GPIO_WriteBit(GPIOC, GPIO_Pin_0, ba);
			GPIO_WriteBit(GPIOC, GPIO_Pin_1, ba);
#else
			GPIO_WriteBit(GPIOC, GPIO_Pin_7, ba);
#endif
			break;
		default:
			break;
	}
}