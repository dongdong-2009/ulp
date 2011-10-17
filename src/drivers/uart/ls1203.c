/*
 * 	2011 initial version
 * Author: david
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

int ls1203_Read(ls1203_t *chip, char *pdata)
{
	for (int i=0;i<chip->data_len;i++) {
		if (chip->bus->poll() == 1) {
			pdata[i] == chip->bus->getchar();
			if (i == chip->data_len - 1 && chip->bus->poll() == 0)
				return 1;
		}
		return 0;
	}
	return 0;
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static ls1203_t pdi_ls1203 = {
	.bus = &uart2,
	.data_len = 19,
	.dead_time = 20,
};


static int cmd_ls1203_func(int argc, char *argv[])
{
	char *barcode;

	const char *usage = { \
		" usage:\n" \
		" ls1203 init,ls1203 debug \n" \
		" ls1203 read,scan barcode \n" \
	};

	if(argc == 1) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		ls1203_Init(&pdi_ls1203);
		printf("init ok \n");
	}

	if (argv[1][0] == 'r') {
		if (ls1203_Read(&pdi_ls1203, barcode))
			printf("%s\r", barcode);
		//else
			//printf("the length of the barcode is not right");
                return 1;
	}

	return 0;
}
const cmd_t cmd_ls1203 = {"ls1203", cmd_ls1203_func, "ls1203 cmd"};
DECLARE_SHELL_CMD(cmd_ls1203)
#endif
