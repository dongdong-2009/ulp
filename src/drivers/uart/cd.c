/* cd.c
 * 	david@2011 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "cd.h"

void cd_Init(cd_t *dev)
{
	uart_cfg_t cfg = {
		.baud = 2400,
	};

	dev->bus->init(&cfg);

	//init custom display
	dev->bus->putchar(0x1B);
	dev->bus->putchar(0x40);
}

int cd_Clr(cd_t *dev)
{
	dev->bus->putchar(0x0D);
	dev->bus->putchar(0x0C);
	return 0;
}

int cd_Send(cd_t *dev, unsigned char *data, int length)
{
	int i;

	dev->bus->putchar(0x0D);
	dev->bus->putchar(0x1B);
	dev->bus->putchar(0x51);
	dev->bus->putchar(0x41);

	for (i = 0; i < length; i++) {
		if ((data[i] > '9') || (data[i] < '0')) {
			dev->bus->putchar(0x0D);
			cd_Clr(dev);
			return 1;
		}
		dev->bus->putchar(data[i]);
	}

	dev->bus->putchar(0x0D);
	return 0;
}

int cd_SetIndicationLight(cd_t *dev, int mode)
{
	dev->bus->putchar(0x1B);
	dev->bus->putchar(0x73);

	dev->bus->putchar(mode);
	return 0;
}

int cd_SetBaud(cd_t *dev, int baud)
{
	uart_cfg_t cfg;
	int n;

	switch (baud) {
		case 9600:
			n = 0x30;
			break;
		case 4800:
			n = 0x31;
			break;
		case 2400:
			n = 0x32;
			break;
		default:
			return 1;
	}
	cfg.baud = baud;

	dev->bus->putchar(0x02);
	dev->bus->putchar(0x42);
	dev->bus->putchar(n);

	dev->bus->init(&cfg);
	return 0;
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static cd_t dbg_cd = {
	.bus = &uart2
};

unsigned char vh_value[8];
int vh_length;
int vh_flash_state = 0;
time_t flash_time;

static int cmd_cd_func(int argc, char *argv[])
{
	int i;

	const char *usage = { \
		" usage:\n" \
		" cd init,         custom display init \n" \
		" cd clr,          clear custom display \n" \
		" cd il n,         set the indication light, n:'0'-'4' \n" \
		" cd send d1 d2.., set the data, dn:30h-39h \n" \
		" cd baud n,       set baud, baud:9600,4800,2400\n" \
		" cd flash,        flash display the number\n" \
	};

	if (argc > 1) {
		if (argv[1][0] == 'i') {
			cd_Init(&dbg_cd);
			printf("init ok \n");
			return 0;
		}

		if (argv[1][0] == 'c') {
			cd_Clr(&dbg_cd);
			return 0;
		}

		if (argv[1][0] == 'l') {
			cd_SetIndicationLight(&dbg_cd, argv[2][0]);
			return 0;
		}

		if (argv[1][0] == 's') {
			vh_length = argc -2;
			for (i = 0; i < vh_length; i++)
				vh_value[i] = argv[i+2][0];
			cd_Send(&dbg_cd, vh_value, vh_length);
			return 0;
		}

		if (argv[1][0] == 'b') {
			cd_SetBaud(&dbg_cd, (unsigned int)atoi(argv[2]));
			return 0;
		}

	}

	if (argc == 0 || argv[1][0] == 'f') {
		if (argc != 0) {
			flash_time = time_get(250);
			cd_Send(&dbg_cd, vh_value, vh_length);
			vh_flash_state = 1;
			return 1;
		}

		if (vh_flash_state) {
			if (time_left(flash_time) > 0) {
				return 1;
			} else {
				cd_Clr(&dbg_cd);
				vh_flash_state = 0;
				flash_time = time_get(250);
				return 1;
			}
		} else {
			if (time_left(flash_time) > 0) {
				return 1;
			} else {
				cd_Send(&dbg_cd, vh_value, vh_length);
				vh_flash_state = 1;
				flash_time = time_get(250);
				return 1;
			}
		}
	}

	printf(usage);
	return 0;
}
const cmd_t cmd_cd = {"cd", cmd_cd_func, "custom display cmd"};
DECLARE_SHELL_CMD(cmd_cd)
#endif
