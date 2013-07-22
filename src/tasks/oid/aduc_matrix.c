/*
 * 	miaofng@2013-7-6 initial version
 *	- support for oid hw version 2.1
 *
 *
 */
#include "ulp/sys.h"
#include "mbi5025.h"
#include "shell/cmd.h"
#include <stdlib.h>
#include "aduc_matrix.h"

/*msb four relay are reserved for mode switch purpose*/
#define MATRIX_ALL 0x0FFF
#define RELAY_ALL 0xFFFF
#define RELAY_OFF 0x0000

static const mbi5025_t matrix_mbi = {
	.bus = &spi0,
	.load_pin = SPI_CS_P00,
	.oe_pin = SPI_CS_NONE,
};

void matrix_init(void)
{
	mbi5025_Init(&matrix_mbi);
	matrix_relay(RELAY_OFF, RELAY_ALL);
}

void matrix_relay(int image, int mask)
{
	static int matrix_image = 0;
	matrix_image &= ~ mask;
	matrix_image |= (image & mask);
	mbi5025_write_and_latch(&matrix_mbi, &matrix_image, 2);
}

void matrix_pick(int pin0, int pin1)
{
	int image = 0;
	pin0 <<= 1;
	pin1 <<= 1;
	image |= 1 << (pin0 + 0);
	image |= 1 << (pin1 + 1);
	matrix_relay(image, MATRIX_ALL);
}

static int cmd_matrix_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"matrix -i			matrix init\n"
		"matrix -w 0x0002 [0xffff]	turn on/off specified matrix relay\n"
		"matrix -p 1 2			turn on/off matrix relay to pick pin1&pin2\n"
	};

	if(argc > 0) {
		int image, mask, pin0, pin1, e = 0;
		for(int j, i = 1; (i < argc) && (e == 0); i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'i':
				matrix_init();
				break;
			case 'w':
				e = 1; mask = 0xffff;
				if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
					image = htoi(argv[++ i]);
					e = 0;
				}
				if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
					mask = htoi(argv[++ i]);
				}

				if(!e) {
					matrix_relay(image, mask);
				}
				break;

			case 'p':
				e = 2;
				if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
					pin0 = atoi(argv[++ i]);
					e --;
				}
				if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
					pin1 = atoi(argv[++ i]);
					e --;
				}

				if(!e) {
					matrix_pick(pin0, pin1);
				}
				break;

			default:
				e ++;
			}
		}

		if(e) {
			printf("%s", usage);
			return 0;
		}
	}
	return 0;
}

const cmd_t cmd_matrix = {"matrix", cmd_matrix_func, "matrix debug cmds"};
DECLARE_SHELL_CMD(cmd_matrix)
