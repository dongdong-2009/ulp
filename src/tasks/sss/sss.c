#include "can.h"
#include "flash.h"
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"
#include "linux/list.h"
#include "sys/sys.h"
#include "sss.h"
#include "sys/task.h"
#include "stm32f10x.h"

static const can_bus_t *can_bus;
static const can_bus_t *can_sss;
extern int card_address;
extern char readdata[8];
can_msg_t can_msg1;
unsigned char can_data[8];

char can1_int(void)
{
	can_sss = &can1;
	can_cfg_t cfg={500000,0};
	can_sss -> init(&cfg);
	RESET_Init();//PCB BUG
	return 0;
}

void ADDRESS_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//°å¿¨LED²¿·Ö
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
	//PCB Bug
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void RESET_Init(void)
{
	//PCB Bug
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static int cmd_sss_func(int argc, char *argv[])
{
	int x;
	can_msg_t msg;
	const char *usage = {
		"usage:\n"
		"sss send id d0 d1 d2 d3 d4 d5 d6 d7 d8    sss send,  id (0001)\n"
	};
	if(argc > 1) {
		if(argv[1][0] == 's') {//can send/t
			can_bus = &can1;
			can_cfg_t cfg={500000,0};
			can_bus -> init(&cfg);
			msg.dlc = argc - 3;
			msg.flag = (argv[1][3] == 'd') ? 0 : CAN_FLAG_EXT;
			if (argc > 3)
				sscanf(argv[2], "%x", &msg.id); //id
			else
				return 0;
			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[3 + x], "%x", (int *)&msg.data[x]);
			}
			if (can_bus -> send(&msg)) {
				printf("uart send fail\n");
			} else {
				printf("      send   success!!\n");
			}
			return 0;
		}
	}
	printf("%s", usage);
	return 0;
}

const cmd_t cmd_sss = {"sss", cmd_sss_func, "can monitor/debugger"};
DECLARE_SHELL_CMD(cmd_sss)

char can_updata(void)
{
	int reset=1;
	reset = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);
	if(reset==0)
		*((u32 *)0xE000ED0C) = 0x05fa0004;
	if(!can_sss -> recv(&can_msg1)) {
		memcpy(can_data,can_msg1.data,can_msg1.dlc);
		if(can_data[0]==0xee)
			printf("the sensor is not exsit! \r\n");
		else if(can_data[0]==0xaa)
			printf("the command is invalid! \r\n");
		else if (can_data[0]==0xbb)
			printf("the card has been reseted to  original state! \r\n ");
		else {
			if(can_msg1.id==0x0002) {
				char i;
				printf("Verification ID: 0x");
				for(i=3;i>=0;i--)
					printf("%x",can_msg1.data[i]);
			}
			else
				printf("this card is simulating sensor %d !\r\n",can_msg1.data[2]);
		}
	}
	return 0;
}

int main(void)
{
	task_Init();
	can1_int();
        ADDRESS_GPIO_Init();
	while(1) {
		task_Update();
		can_updata();
	}
}
