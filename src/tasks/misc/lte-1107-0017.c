#include "stm32f10x.h"
#include "sys/task.h"
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"
#include "time.h"
#include "sys/sys.h"

char card_address;
can_msg_t recv_msg;
int recv_data;

void Address_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

char Address_Read(void)
{
	char adress = 0;
	adress += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
	adress = (adress << 1);
	adress += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
	adress = (adress << 1);
	adress += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
	adress = (adress << 1);
	adress += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
	adress = (adress << 1);
	adress += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15);
	return (adress);
}

void Channel_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	if(card_address == 0x1f);
	else{
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_8;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
	}
}

void relay_on(int channel)
{
	if((channel >> 0) & 0x01)
		GPIO_SetBits(GPIOA, GPIO_Pin_3);
	if((channel >> 1) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_4);
	if((channel >> 2) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_5);
	if((channel >> 3) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_0);
	if((channel >> 4) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_1);
	if((channel >> 5) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_10);
	if((channel >> 6) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_11);
	if((channel >> 7) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_12);
	if((channel >> 8) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_13);
	if((channel >> 9) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_14);
	if((channel >> 10) & 0x01)
		GPIO_SetBits(GPIOB, GPIO_Pin_15);
	if((channel >> 11) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_6);
	if((channel >> 12) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_7);
	if((channel >> 13) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_8);
	if((channel >> 14) & 0x01)
		GPIO_SetBits(GPIOC, GPIO_Pin_9);
	if((channel >> 15) & 0x01)
		GPIO_SetBits(GPIOA, GPIO_Pin_8);
}

void relay_off(int channel)
{
	if((channel >> 0) & 0x01)
		GPIO_ResetBits(GPIOA, GPIO_Pin_3);
	if((channel >> 1) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_4);
	if((channel >> 2) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_5);
	if((channel >> 3) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_0);
	if((channel >> 4) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_1);
	if((channel >> 5) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);
	if((channel >> 6) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_11);
	if((channel >> 7) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	if((channel >> 8) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	if((channel >> 9) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_14);
	if((channel >> 10) & 0x01)
		GPIO_ResetBits(GPIOB, GPIO_Pin_15);
	if((channel >> 11) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_6);
	if((channel >> 12) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_7);
	if((channel >> 13) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_8);
	if((channel >> 14) & 0x01)
		GPIO_ResetBits(GPIOC, GPIO_Pin_9);
	if((channel >> 15) & 0x01)
		GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

static const can_bus_t *can_bus;
void Can1_Init(void)
{
	can_bus = &can1;
	can_cfg_t cfg = {500000, 0};
	can_bus -> init(&cfg);
	can_filter_t filter[] = {
		{
			.id = (int)card_address,
			.mask=  0xffff,
			.flag = 0,
		}
	};
	can_bus -> filt(filter, 1);
}

static int cmd_relay_func(int argc, char *argv[])
{
	int data;
	can_msg_t msg;
	msg.dlc = 3;
	const char *usage = {
		"usage:\n"
		"  relay set card_number channel\n"
		"  relay reset card_number channel\n"
		"  relay query card_number channel\n"
	};

	if(argc != 4){
		printf("error: command is wrong!!\n");
		printf("%s", usage);
	}
	else{
		sscanf(argv[2], "%d", &msg.id);
		data = cmd_ptget(argv[3]);
		if((data > 0xffff) || (msg.id > 12) || ( msg.id > 8 && data > 0xf ))
			printf("error: command is wrong!!\n");
		else{
			msg.data[1] = (char)(data >> 8);
			msg.data[2] = (char)data;
			if(!strcmp(argv[1], "on")){
				msg.data[0] = 0;
				if(can_bus -> send(&msg))
					printf("      send fail!!\n");
				else
					printf("      send success!!\n");
			}else if(!strcmp(argv[1], "off")){
				msg.data[0] = 1;
				if(can_bus -> send(&msg))
					printf("      send fail!!\n");
				else
					printf("      send success!!\n");
			}else if(!strcmp(argv[1], "query")){
				msg.data[0] = 2;
				if(can_bus -> send(&msg))
					printf("      send fail!!\n");
				else
					printf("      send success!!\n");
			}
		}
	}
	return 0;
}

const cmd_t cmd_relay = {"relay", cmd_relay_func, "control relay"};
DECLARE_SHELL_CMD(cmd_relay)

void Slu_Init()
{
	Address_Init();
	card_address = Address_Read();
	Channel_Init();
	Can1_Init();
}

void Slu_Update()
{
	if(!can_bus -> recv(&recv_msg)){
		memcpy(recv_data, recv_msg.data, recv_msg.dlc);
	}
}

DECLARE_TASK(Slu_Init, Slu_Update)