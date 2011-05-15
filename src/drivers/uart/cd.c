/* cd.c
 * 	david@2011 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "cd.h"

void cd_Init(cd_t *dev)
{
	uart_cfg_t cfg = { //UART_CFG_DEF;
		.baud = 2400,
	};

	dev->bus->init(&cfg);

	//init custom display
	dev->bus->putchar(0x1B);
	dev->bus->putchar(0x40);
}

int cd_Clr(cd_t *dev)
{
	dev->bus->putchar(0x0C);
	return 0;
}

int cd_Send(cd_t *dev, unsigned char *data, int length)
{
	int i;

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
}

#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>

static cd_t dbg_cd = {
	.bus = &uart2
};


static int cmd_cd_func(int argc, char *argv[])
{
	unsigned char temp[8];
	int i;

	const char *usage = { \
		" usage:\n" \
		" cd init,         custom display init \n" \
		" cd clr,          clear custom display \n" \
		" cd il n,         set the indication light, n:'0'-'4' \n" \
		" cd send d1 d2.., set the data, dn:30h-39h \n" \
		" cd baud n,       set baud, baud:9600,4800,2400\n" \
	};

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	if (argv[1][0] == 'i') {
		cd_Init(&dbg_cd);
		printf("init ok \n");
	}

	if (argv[1][0] == 'c') {
		cd_Clr(&dbg_cd);
	}

	if (argv[1][0] == 'i') {
		cd_SetIndicationLight(&dbg_cd, argv[2][0]);

	}

	if (argv[1][0] == 's') {
		for (i = 0; i < argc -2; i++)
			temp[i] = argv[i+2];
		cd_Send(&dbg_cd, temp, argc -2);
	}

	if (argv[1][0] == 'b') {
		cd_SetBaud(&dbg_cd, atoi(argv[2]));
	}

	return 0;
}
const cmd_t cmd_cd = {"cd", cmd_cd_func, "custom display cmd"};
DECLARE_SHELL_CMD(cmd_cd)
#endif
