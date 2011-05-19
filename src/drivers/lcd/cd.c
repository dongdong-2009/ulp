/* cd.c
 * 	david@2011 initial version
 */

#include "config.h"
#include "stm32f10x.h"
#include "string.h"
#include "cd.h"
#include "driver.h"
#include "lcd.h"
#include "uart.h"
#include "time.h"

enum {
	MODE_IL0 = 0x30,
	MODE_IL1,
	MODE_IL2,
	MODE_IL3,
	MODE_IL4
};

static uart_bus_t *cd_bus;
static unsigned char cd_ram[9];
static cd_Send(unsigned char *data, int length);
static int cd_SetIndicationLight(int mode);

int cd_Init(const struct lcd_cfg_s *cfg)
{
	cfg = cfg;
	memset(cd_ram, 0, sizeof(cd_ram));
	//init custom display
	cd_bus->putchar(0x1B);
	cd_bus->putchar(0x40);
	return 0;
}

static int cd_Send(unsigned char *data, int length)
{
	int i;

	cd_bus->putchar(0x0D);
	cd_bus->putchar(0x1B);
	cd_bus->putchar(0x51);
	cd_bus->putchar(0x41);

	for (i = 0; i < length; i++) {
		cd_bus->putchar(data[i]);
	}

	cd_bus->putchar(0x0D);
	return 0;
}

static int cd_SetIndicationLight(int mode)
{
	cd_bus->putchar(0x1B);
	cd_bus->putchar(0x73);

	cd_bus->putchar(mode);

	return 0;
}

int cd_Clr()
{
	cd_bus->putchar(0x0D);
	cd_bus->putchar(0x0C);
	return 0;
}

//cd_ram[0]:indicator
//cd_ram[1] - cd_ram[8]:seven seg
int cd_WriteString(int column, int row, const char *s)
{
	int i,size;
	size = strlen(s);

	if(row > 0)
		return 0;

	for(i = 0; i < size; i++)
		cd_ram[i + column] = (unsigned char)s[i];

	if (column == 0) {
		switch (cd_ram[0]) {
			case '0':
				cd_SetIndicationLight(MODE_IL0);
				break;
			case '1':
				cd_SetIndicationLight(MODE_IL1);
				break;
			case '2':
				cd_SetIndicationLight(MODE_IL2);
				break;
			case '3':
				cd_SetIndicationLight(MODE_IL3);
				break;
			case '4':
				cd_SetIndicationLight(MODE_IL4);
				break;
			default:
				break;
		}
	}

	cd_Send(&cd_ram[1], 8);
	return 0;
}

int cd_SetBaud(int baud)
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

	cd_bus->putchar(0x02);
	cd_bus->putchar(0x42);
	cd_bus->putchar(n);

	cd_bus->init(&cfg);
	return 0;
}

static const struct lcd_dev_s custom_display = {
	.xres = 1,
	.yres = 9,
	.init = cd_Init,
	.puts = cd_WriteString,

	.setwindow = NULL,
	.rgram = NULL,
	.wgram = NULL,

	.writereg = NULL,
	.readreg = NULL,
};

static void custom_display_reg(void)
{
	uart_cfg_t cfg = {
		.baud = 2400,
	};

	cd_bus = &uart2;
	cd_bus->init(&cfg);

	lcd_add(&custom_display, "custom display", LCD_TYPE_CHAR);
}
driver_init(custom_display_reg);

#if 0
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
