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
#include "nvm.h"

#define INBOX_ADDR	0x0F000000
#define OUTBOX_ADDR	0x0F000100

#define ID	0x7ee
#define WRITE	1
#define READ	2
#define SAVE	3

static char data[2048] __nvm;

static mcamos_srv_t server;

void Fixture_Write()
{
	unsigned short offset = 0;
	unsigned short length = 0;
	memcpy(&length, server.inbox + 1, 1);
	memcpy(&offset, server.inbox + 2, 2);
	if(length < 61 && (offset + length) < 2049){
		memcpy(data + offset, server.inbox + 4, length);
		server.outbox[0] = server.inbox[0];
		server.outbox[1] = server.inbox[1];
	}
	else{
		server.outbox[1] = 0;
	}
}

void Fixture_Read()
{
	unsigned short offset = 0;
	unsigned short length = 0;
	memcpy(&length, server.inbox + 1, 1);
	memcpy(&offset, server.inbox + 2, 2);
	if(server.inbox[3] < 61 && (offset + length) < 2049){
		memcpy(server.outbox + 2, data + offset, length);
		server.outbox[0] = server.inbox[0];
		server.outbox[1] = server.inbox[1];
	}
	else{
		server.outbox[1] = 0;
	}
}

void Fixture_Save()
{
	nvm_save();
}


void Server_Init(void)
{
	int id = ID;
	server.can = &can1;
	server.id_cmd = id;
	server.id_dat = id + 1;
	server.baud = 500000;
	server.timeout = 8;
	server.inbox_addr = INBOX_ADDR;
	server.outbox_addr = OUTBOX_ADDR;
	mcamos_srv_init(&server);
}

void Server_Update(void)
{
	mcamos_srv_update(&server);

	switch(server.inbox[0]) {
	case WRITE:
		Fixture_Write();
		break;
	case READ:
		Fixture_Read();
		break;
	case SAVE:
		Fixture_Save();
		break;
	default:
		break;
	}
	server.inbox[0] = 0;
}

void main()
{
	task_Init();
	Server_Init();
	while(1){
		task_Update();
		Server_Update();
	}
}