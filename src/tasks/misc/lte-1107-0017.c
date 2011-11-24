#include "stm32f10x.h"
#include "sys/task.h"
#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"
#include "ulp_time.h"
#include "sys/sys.h"
#include "priv/mcamos.h"
#include "led.h"
#include "nvm.h"

#define SLU_INBOX_ADDR 0x0F000000
#define SLU_OUTBOX_ADDR 0x0F000100
#define SET 1
#define GET 2
#define ON 1
#define OFF 2

static int channel_num __nvm;
char card_address;
unsigned int relay_status = 0;

struct slu_data_s{
	int data;
	char status;
};

unsigned char return_data[6];

void Led_R_On(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
}

void Led_G_On(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_10);
}

void Led_R_Off(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_12);
}

void Led_G_Off(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_10);
}

void Address_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

char Address_Read(void)
{
	char address = 0;
	address += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
	address = (address << 1);
	address += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
	address = (address << 1);
	address += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1);
	address = (address << 1);
	address += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
	address = (address << 1);
	address += GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15);
	return (address);
}

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

static int slu_wait(struct mcamos_s *m, int timeout)
{
	int ret;
	char cmd;
	time_t deadline = time_get(timeout);

	do {
		ret = mcamos_upload(m ->can, SLU_INBOX_ADDR, &cmd, 1, 10);
		if(ret) {
			return -1;
		}

		if(time_left(deadline) < 0) {
			return -1;
		}
	} while(cmd != 0);
	ret = mcamos_upload(m->can, SLU_OUTBOX_ADDR, (char*)return_data, 6, 10);
	if(ret || (char)return_data[1] == -1){
		return -1;
	}

	return 0;
}

int Slu_Set(int card_num, const struct slu_data_s *data)
{
	int ret = -1;
	char cmd = SET;
	struct mcamos_s m;
	m.can = &can1;
	m.baud = 500000;
	m.id_cmd = card_num;
	m.id_dat = m.id_cmd + 20;

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, SLU_INBOX_ADDR + 1, (char *) data, sizeof(struct slu_data_s ), 10);
	mcamos_dnload(m.can, SLU_INBOX_ADDR, &cmd, 1, 10);
	ret = slu_wait(&m, 10);
	return ret;
}

int Slu_Get(int card_num, const struct slu_data_s *data)
{
	int ret = -1;
	char cmd = GET;
	struct mcamos_s m;
	m.can = &can1;
	m.baud = 500000;
	m.id_cmd = card_num;
	m.id_dat = m.id_cmd + 20;

	mcamos_init_ex(&m);
	mcamos_dnload(m.can, SLU_INBOX_ADDR, &cmd, 1, 10);
	ret = slu_wait(&m, 10);
	relay_status = (unsigned int)return_data[5];
	relay_status <<= 8;
	relay_status += (unsigned int)(return_data[4]);
	relay_status <<= 8;
	relay_status += (unsigned int)(return_data[3]);
	relay_status <<= 8;
	relay_status += (unsigned int)(return_data[2]);
	return ret;
}

static mcamos_srv_t slu_server;

void Server_Init(void)
{
	int slu_id = (int)card_address;
	slu_server.can = &can1;
	slu_server.id_cmd = slu_id;
	slu_server.id_dat = slu_id + 20;
	slu_server.baud = 500000;
	slu_server.timeout = 8;
	slu_server.inbox_addr = SLU_INBOX_ADDR;
	slu_server.outbox_addr = SLU_OUTBOX_ADDR;
	mcamos_srv_init(&slu_server);
}

void Server_Update(void)
{
	int channel_num_temp;
	char ret = 0;
	struct slu_data_s data;
	unsigned int channel;
	char *inbox = slu_server.inbox;
	channel_num_temp = 1 << channel_num;
	Led_G_On();
	/*if(relay_status)
		Led_R_On();
	else
		Led_R_Off();*/
	mcamos_srv_update(&slu_server);
	switch(inbox[0]) {
	case SET:
		memcpy(&data, slu_server.inbox + 1, sizeof(struct slu_data_s));
		channel = data.data;
		if(channel >= channel_num_temp)
			ret = -1;
		else{
			if(data.status == ON)
				relay_on(channel);
			else if(data.status == OFF)
				relay_off(channel);
			else
				return;
			break;
		}
		
	case GET:
		memcpy(&(slu_server.outbox[2]), &relay_status, sizeof(relay_status));
		break;
		
	case 0:
		return;
	}

	slu_server.outbox[0] = inbox[0];
	slu_server.outbox[1] = ret;
	inbox[0] = 0;
}

void display_status(int status)
{
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
	struct slu_data_s slu_data;
	int ret = -1;
	int id = 0;
	const char *usage = {
		"usage:\n"
		"  relay set card_number channel on/off\n"
		"  relay get card_number\n"
	};

	if(argc != 3 && argc != 5){
		printf("error: command is wrong!!\n");
		printf("%s", usage);
		return -1;
	}
	else{
		if(argc == 5 && !strcmp(argv[1], "set")){
			sscanf(argv[2], "%d", &id);
			slu_data.data = cmd_pattern_get(argv[3]);
			if(id > 20 || (strcmp(argv[4], "on") && strcmp(argv[4], "off"))){
				printf("error: command is wrong!!\n");
				return -1;
			}
			else{
				if(!strcmp(argv[4], "on")){
					slu_data.status = ON;
					ret = Slu_Set(id, &slu_data);
				}
				else if(!strcmp(argv[4], "off")){
					slu_data.status = OFF;
					ret = Slu_Set(id, &slu_data);
				}
				if(ret){
					printf("      operation fail!!\n");
					return -1;
				}
				else{
					printf("      operation success!!\n");
					return 0;
				}
			}
		}
		else if(argc == 3 && !strcmp(argv[1], "get")){
			sscanf(argv[2], "%d", &id);
			if(id > 20){
				printf("error: command is wrong!!\n");
				return -1;
			}
			else{
				if(Slu_Get(id, &slu_data)){
					printf("      operation fail!!\n");
					return -1;
				}
				else{
					printf("      operation success!!\n");
					display_status(relay_status);
					return 0;
				}
			}	
		}
		else if(argc == 3 && !strcmp(argv[1], "save")){
			sscanf(argv[2], "%d", &id);
			channel_num = id;
			if(nvm_save()){
				printf("      operation fail!!\n");
				return -1;
			}
			else{
				printf("      operation success!!\n");
				return 0;
			}
		}
	}
	return -1;
}

const cmd_t cmd_relay = {"relay", cmd_relay_func, "control relay"};
DECLARE_SHELL_CMD(cmd_relay)

void Slu_Init()
{
	Address_Init();
	card_address = Address_Read();
	if(card_address != 0x1f){
		Channel_Init();
		Server_Init();
	}	
}

void Slu_Update()
{
	if(card_address != 0x1f) Server_Update();
}

void main()
{
	task_Init();
	Slu_Init();
	while(1){
		task_Update();
		Slu_Update();
	}
}