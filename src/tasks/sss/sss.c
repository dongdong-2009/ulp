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
#include "nvm.h"
#include "priv/mcamos.h"

static int j;
static struct sensor_nvm_s sensor_lib_s[64] __nvm;				//
static char mailbox[64];

void ADDRESS_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//板卡LED部分
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
	const char *usage = {
		"usage:\n"
		"sss send id d0 d1 d2 d3 d4 d5 d6 d7 d8    sss send,  id (0001)\n"
	};
	if(argc == 2) {
		if(!strcmp(argv[1],"save"))
			nvm_save();
		else if(!strcmp(argv[1],"list"))
			for(int i=0;i<128;i++)
				printf(sensor_lib_s[i].name,"\r",sensor_lib_s[i].ID,"\n");
		else
			printf("invalid command \n");
		return 0;
	}
	if(argc == 3) {
		if(!strcmp(argv[1],"query")) {
			char cmd = SSS_CMD_QUERY;
			struct mcamos_s m;
			m.can = &can1;
			m.baud = 500000;
			m.id_cmd = (int)argv[2];
			m.id_dat = (int)argv[2] + 24;
			mcamos_init_ex(&m);
			mcamos_dnload(&can1,SSS_INBOX_ADDR ,&cmd,1,10);

		}
		else if(1)
			printf("invalid command \n");
		return 0;
	}
	if(argc == 4) {
		if(!strcmp(argv[1],"learn")) {
			char cmd ;
			struct mcamos_s m;
			m.can = &can1;
			m.baud = 500000;
			m.id_cmd = (int)argv[2];
			m.id_dat = (int)argv[2] + 24;
			mcamos_init_ex(&m);
			mailbox[0] = SSS_CMD_LEARN;
			mcamos_dnload(&can1,SSS_INBOX_ADDR , mailbox, 1, 10);
			do {
				mcamos_upload(&can1,SSS_INBOX_ADDR, &cmd, 1, 10);
			} while(cmd != 0);
			mcamos_upload(&can1,SSS_OUTBOX_ADDR + 1, mailbox, 1, 10);//如何
			return 0;
		}
		else if(!strcmp(argv[1],"select")) {
			char cmd = SSS_CMD_SELECT;
			struct mcamos_s m;
			m.can = &can1;
			m.baud = 500000;
			m.id_cmd = (int)argv[2];
			m.id_dat = (int)argv[2] + 24;
			mcamos_init_ex(&m);
			mailbox[0] = cmd;
			for(j = 0;j <= 128;j ++) {
 				if(j == 128)
					printf("The sensor is not exsit \n");
				if(strcmp(argv[3],sensor_lib_s[j].name) == 0) {
					//mailbox[0] = cmd;
					memcpy(mailbox + 1, &sensor_lib_s[j], sizeof(struct sensor_nvm_s));
					mcamos_dnload(&can1,SSS_INBOX_ADDR + 1,mailbox + 1,sizeof(struct sensor_nvm_s),10);
					mcamos_dnload(&can1,SSS_INBOX_ADDR ,mailbox ,1,10);

				mcamos_upload(&can1 ,SSS_INBOX_ADDR ,mailbox ,1,10);
				if(mailbox[0] == 0) {
					printf("send success \n");
				}
                                }
			}
			return 0;
		}
		else printf("invalid command \n");
	}
	/* if(argc > 1) {
		if(argv[1][0] == 'l' && argv[1][3] != 't') {	//learn or select
			can_bus = &can1;
			can_cfg_t cfg={500000,0};
			can_bus -> init(&cfg);
			msg.dlc = argc - 2;
			msg.flag = 0;
			//msg.id = 0001;//学习时发过去的id
			sscanf(argv[1], "%x", &msg.id); //id
			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[2 + x], "%x", (int *)&msg.data[x]);
			}
 			if (can_bus -> send(&msg)) {
				printf("uart send fail\n");
			} else {
				printf("      send   success!!\n");
			}
			return 0;
		}
		if(argv[1][0] == 's' && argv[1][3] != 'e') {	//select
			can_bus = &can1;
			can_cfg_t cfg={500000,0};
			can_bus -> init(&cfg);
			msg.dlc = argc - 2;
			msg.flag = 0;
			msg.id = 0002;//选择时发过去的id
			//sscanf(argv[1], "%x", &msg.id); //id
			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[2 + x], "%x", (int *)&msg.data[x]);
			}
			if (can_bus -> send(&msg)) {
				printf("uart send fail\n");
			} else {
				printf("      send   success!!\n");
			}
			return 0;
		}
		if(argv[1][0] == 'q') {	//learn or select
			can_bus = &can1;
			can_cfg_t cfg={500000,0};
			can_bus -> init(&cfg);
			msg.dlc = argc - 2;
			msg.flag = 0;
			msg.id = 0003;//查询模式发过去的id
			//sscanf(argv[1], "%x", &msg.id); //id
			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[2 + x], "%x", (int *)&msg.data[x]);
			}
			if (can_bus -> send(&msg)) {
				printf("uart send fail\n");
			} else {
				printf("      send   success!!\n");
			}
			return 0;
		}
		if(argv[1][0] == 'l' && argc == 2) {		//list
			for(int i=0;i<128;i++) {
				printf(sensor_lib_s[i].name,"\r");
				printf(sensor_lib_s[i].ID,"\r");
				printf(sensor_lib_s[i].speed,"\n");
			}
		}
		if(argv[1][0] == 'c') {				//config
			sscanf(argv[1], "%x", (int *)&sensors.name);
			sscanf(argv[2], "%x", (int *)&sensors.ID);
								//sensors.speed = 0;
		}
		if(argv[1][0] == 's' && argc == 2) {		//save

		} */
	printf("%s", usage);
	return 0;
}

const cmd_t cmd_sss = {"sss", cmd_sss_func, "can monitor/debugger"};
DECLARE_SHELL_CMD(cmd_sss)

/* char can_updata (void)
{
	int reset=1;
	reset = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);
	if (reset==0)
		*((u32 *)0xE000ED0C) = 0x05fa0004;
	if (!can_sss -> recv(&can_msg1)) {
		memcpy(can_data,can_msg1.data,can_msg1.dlc);
		if (can_data[0]==0xee)
			printf("the sensor is not exsit! \r\n");
		else if (can_data[0]==0xaa)
			printf("the command is invalid! \r\n");
		else if (can_data[0]==0xbb)
			printf("the card has been reseted to  original state! \r\n ");
		else if (can_msg1.id==0x0002) {
				char i;
				printf("Verification ID: 0x");
				for(i=3;i>=0;i--)
					printf("%x",can_msg1.data[i]);
			}
		printf("this card is simulating sensor %d !\r\n",can_msg1.data[2]);
	}
	return 0;
} */

int main(void)
{
	task_Init();
	//can1_int();
        ADDRESS_GPIO_Init();
	while(1) {
		task_Update();
		//can_updata();
	}
}
