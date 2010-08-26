/*
*	dusk@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"

void usblower_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	//USB PULL-UP resister GPIO config
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	
#ifdef CONFIG_USB_PULLUP_PD2
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_2);
#endif

	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = USB_HP_CAN1_TX_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void usblower_PullupEnable(void)
{
#ifdef CONFIG_USB_PULLUP_PD2
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
#endif
}

void usblower_PullupDisable(void)
{
#ifdef CONFIG_USB_PULLUP_PD2
	GPIO_SetBits(GPIOD, GPIO_Pin_2);
#endif
}
