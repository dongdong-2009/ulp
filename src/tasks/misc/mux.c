#include "stm32f10x.h"
#include "sys/task.h"
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulp_time.h"
#include "sys/sys.h"
#include <ctype.h>


#define SET 1
#define GET 2
#define ON 1
#define OFF 2
#define IMAGE 3

unsigned int relay_status = 0;

void Channel_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_12;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void relay_on(unsigned int channel)
{
	relay_status |= channel;
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

void relay_off(unsigned int channel)
{
	relay_status &= ~channel;
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

void display_status(int status)
{
	if(status == 0) {
		printf("      No channel is on now!!\n");
		return;
	}
	if((status >> 0) & 0x0001)
		printf("      Channel 0 is on now!!\n");
	if((status >> 1) & 0x0001)
		printf("      Channel 1 is on now!!\n");
	if((status >> 2) & 0x0001)
		printf("      Channel 2 is on now!!\n");
	if((status >> 3) & 0x0001)
		printf("      Channel 3 is on now!!\n");
	if((status >> 4) & 0x0001)
		printf("      Channel 4 is on now!!\n");
	if((status >> 5) & 0x0001)
		printf("      Channel 5 is on now!!\n");
	if((status >> 6) & 0x0001)
		printf("      Channel 6 is on now!!\n");
	if((status >> 7) & 0x0001)
		printf("      Channel 7 is on now!!\n");
	if((status >> 8) & 0x0001)
		printf("      Channel 8 is on now!!\n");
	if((status >> 9) & 0x0001)
		printf("      Channel 9 is on now!!\n");
	if((status >> 10) & 0x0001)
		printf("      Channel 10 is on now!!\n");
	if((status >> 11) & 0x0001)
		printf("      Channel 11 is on now!!\n");
	if((status >> 12) & 0x0001)
		printf("      Channel 12 is on now!!\n");
	if((status >> 13) & 0x0001)
		printf("      Channel 13 is on now!!\n");
	if((status >> 14) & 0x0001)
		printf("      Channel 14 is on now!!\n");
	if((status >> 15) & 0x0001)
		printf("      Channel 15 is on now!!\n");
	if((status >> 16) & 0x0001)
		printf("      Channel 16 is on now!!\n");
	if((status >> 17) & 0x0001)
		printf("      Channel 17 is on now!!\n");
	if((status >> 18) & 0x0001)
		printf("      Channel 18 is on now!!\n");
	if((status >> 19) & 0x0001)
		printf("      Channel 19 is on now!!\n");
	if((status >> 20) & 0x0001)
		printf("      Channel 20 is on now!!\n");
	if((status >> 21) & 0x0001)
		printf("      Channel 21 is on now!!\n");
	if((status >> 22) & 0x0001)
		printf("      Channel 22 is on now!!\n");
	if((status >> 23) & 0x0001)
		printf("      Channel 23 is on now!!\n");
	if((status >> 24) & 0x0001)
		printf("      Channel 24 is on now!!\n");
	if((status >> 25) & 0x0001)
		printf("      Channel 25 is on now!!\n");
	if((status >> 26) & 0x0001)
		printf("      Channel 26 is on now!!\n");
	if((status >> 27) & 0x0001)
		printf("      Channel 27 is on now!!\n");
	if((status >> 28) & 0x0001)
		printf("      Channel 28 is on now!!\n");
	if((status >> 29) & 0x0001)
		printf("      Channel 29 is on now!!\n");
	if((status >> 30) & 0x0001)
		printf("      Channel 30 is on now!!\n");
	if((status >> 31) & 0x0001)
		printf("      Channel 31 is on now!!\n");
}

static int cmd_relay_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"  relay set channel on/off\n"
		"  relay get channel\n"
	};

	if(argc != 3 && argc != 4){
		printf("error: command is wrong!!\n");
		printf("%s", usage);
		return -1;
	}
	else{
		if(argc == 4 && !strcmp(argv[1], "set")){
			int channel = cmd_pattern_get(argv[2]);
			if((strcmp(argv[3], "on") && strcmp(argv[3], "off"))){
				printf("error: command is wrong!!\n");
				return -1;
			}
			else{
				if(!strcmp(argv[3], "on")){
					relay_on(channel);
					printf("OK!!\n");
					return 0;
				}
				else if(!strcmp(argv[3], "off")){
					relay_off(channel);
					printf("OK!!\n");
					return 0;
				}
			}
		}
		else if(argc == 3 && !strcmp(argv[1], "get") && !strcmp(argv[2], "status")){
			display_status(relay_status);
			return 0;
		}
		else {
			printf("error: command is wrong!!\n");
			return -1;
		}
	}
}

const cmd_t cmd_relay = {"relay", cmd_relay_func, "control relay"};
DECLARE_SHELL_CMD(cmd_relay)

void main()
{
	task_Init();
	Channel_Init();
	while(1){
		task_Update();
	}
}