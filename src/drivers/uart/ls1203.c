/*
 * 	2011 initial version
 * Author: peng.guo david
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "ls1203.h"

void ls1203_Init(ls1203_t *chip)
{
	uart_cfg_t cfg = { //UART_CFG_DEF;
		.baud = 9600,
	};

	chip->bus->init(&cfg);

}

int ls1203_Read(ls1203_t *chip, unsigned char *pdata)
{

}

#if 0
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static md204l_t dbg_hid_md204l = {
	.bus = &uart2,
	.station = 0x01,
};


static int cmd_md204l_func(int argc, char *argv[])
{
	short temp = 0;
	int addr;

	const char *usage = { \
		" usage:\n" \
		" md204l init,md204l debug \n" \
		" md204l write regaddr value, regaddr:0-127,value:16bits \n" \
		" md204l read regaddr, regaddr:0-127,value:16bits \n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		md204l_Init(&dbg_hid_md204l);
		printf("init ok \n");
	}

	if (argv[1][0] == 'w') {
		addr = atoi(argv[2]);
		temp = atoi(argv[3]);
		if (md204l_Write(&dbg_hid_md204l, addr, &temp, 1))
			printf("Write error\n");
		else
			printf("Write sucessfully\n");
	}
	if (argv[1][0] == 'r') {
		temp = 0;
		addr = atoi(argv[2]);
		if (md204l_Read(&dbg_hid_md204l, addr, &temp, 1))
			printf("Read error!\n");
		else
			printf("data is: %d\n\r",temp);
	}

	return 0;
}
const cmd_t cmd_md204l = {"md204l", cmd_md204l_func, "md204l cmd"};
DECLARE_SHELL_CMD(cmd_md204l)
#endif
