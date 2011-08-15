/*
 *	sky@2011 initial version
 */
#include "config.h"
#include "sss_card.h"
#include "stm32f10x.h"


int ADDRESS_GPIO_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;
	//板卡地址总线部分
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	//PCB Bug
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	return 0;

}


int ADDRESS_GPIO_Read(void)
{
	int adress=0;
	char scan_bit;
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
	adress|=(scan_bit<<0);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
	adress|=(scan_bit<<1);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
	adress|=(scan_bit<<2);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5);
	adress|=(scan_bit<<3);
	scan_bit = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
	adress|=(scan_bit<<4);
	return (adress);
}

int sensor_poweron(void)

{	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_11);
	return 0;

}

int sensor_poweroff(void)

{	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	return 0;

}

int simulator_poweroff(void)

{        
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_Cmd(TIM2, DISABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_11);
	return 0;

}


int LED_RED_ON(void)
{

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_10);
	GPIO_ResetBits(GPIOC, GPIO_Pin_12);
	return 0;
}


int LED_GREEN_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
	GPIO_ResetBits(GPIOC, GPIO_Pin_10);
	return 0;

}


int LED_MIX_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_10);
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
	return 0;

}

