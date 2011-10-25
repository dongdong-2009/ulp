/*
 * 	2011 initial version
 * Author: david peng.guo
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "ls1203.h"
#include "ulp_time.h"

void ls1203_Init(ls1203_t *chip)
{
	uart_cfg_t cfg = { //UART_CFG_DEF;
		.baud = 9600,
	};

	chip->bus->init(&cfg);
}

int ls1203_Read(ls1203_t *chip, char *code_data)
{
	int i = 0;
	time_t scanner_overtime;

	if (chip->bus->poll()) {
		for (i = 0; i < chip->data_len; i++) {
			scanner_overtime = time_get(chip->dead_time);
			while (chip->bus->poll() == 0) {
				if (time_left(scanner_overtime) < 0)
					return 1;
			}
			code_data[i] = chip->bus->getchar();
		}
		return 0;
	}

	return 1;
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static ls1203_t sr = {
	.bus = &uart2,
	.data_len = 19,
	.dead_time = 20,
};
static char pdi_barcode[19];

static int cmd_ls1203_func(int argc, char *argv[])
{


	const char *usage = { \
		" usage:\n" \
		" ls1203 init,ls1203 debug \n" \
		" ls1203 read,scan barcode \n" \
	};

	if(argc > 0) {
		if(argc < 2){
			printf(usage);
			return 0;
		}

		if (argv[1][0] == 'i') {
			ls1203_Init(&sr);
			printf("init ok \n");
			return 0;
		}

		if (argv[1][0] == 'r') {
			if (ls1203_Read(&sr,pdi_barcode) == 0)
				printf("data is:%s\n", pdi_barcode);
			return 1;
		}
	}

	if(argc == 0) {
		if (ls1203_Read(&sr, pdi_barcode) == 0)
			printf("data is:%s\n", pdi_barcode);
		return 1;
	}

	return 0;
}
const cmd_t cmd_ls1203 = {"ls1203", cmd_ls1203_func, "ls1203 cmd"};
DECLARE_SHELL_CMD(cmd_ls1203)
#endif
