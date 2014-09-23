/*
*  irt route board
*  miaofng@2014-9-18   initial version
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "rut.h"
#include "mbi5025.h"

static const mbi5025_t rut_mbi = {.bus = &spi2, .load_pin = SPI_2_NSS, .oe_pin = SPI_CS_PB12};

/*to conver relay name order(LSn) to mbi5024 control line nr(Km)*/
static int rly_map(int relays)
{
	int image = 0;
	const char map[] = {0,11,12,21,1,10,13,20,  2,9,14,19,5,6,24,3,  18,25,8,15,7,4,26,17,  16,23,22,27,};
	for(int i = 0; i < sizeof(map); i ++) {
		if(relays & (1 << i)) {
			image |= 1 << map[i];
		}
	}
	return image;
}

void rut_init(void)
{
	mbi5025_DisableOE(&rut_mbi);
	mbi5025_Init(&rut_mbi);
	mbi5025_EnableOE(&rut_mbi);
}

/* warnning:
1, RPB: all slot's vline/iline must be shorten and PROBE line resistance will not be ignored!!
2, RMX: the same as rpb, but slot's vline/iline is shorten only when required
3, L4R = W4R, but IS is different
4, L2T could be used for diode test
5, maybe we should delete K4,K5,K6,K7,K20?
*/
void rut_mode(int mode)
{
	#define LS(n) (1 << n)

	const int image[] = {
		LS(12)|LS(14)|LS(27), /*HVR|LV*/
		LS(18)|LS(15)|LS(23)|LS(21)|LS(27), /*L4R|LV, real 4 line resistor measurement*/
		LS(17)|LS(15)|LS(24)|LS(21)|LS(27), /*W4R|LV, use Is!*/
		LS(18)|LS(15)|LS(27), /*L2T|LV, dmm sense pin is open*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(8)|LS(9)|LS(10)|LS(11), /*RPB*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(8)|LS(9)|LS(10)|LS(11), /*RMX*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(4)|LS(5)|LS(6)|LS(7), /*RX2*/
		LS(25)|LS(16)|LS(18), /*VHV*/
		LS(26)|LS(16)|LS(18), /*VLV*/
		LS(17)|LS(16)|LS(13), /*IIS*/
		0x00, /*DBG*/
		0x00, /*OFF*/
	};

	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));
	int relays = image[mode];
	relays = rly_map(relays);
	mbi5025_write_and_latch(&rut_mbi, &relays, sizeof(relays));
}

static int cmd_relay_func(int argc, char *argv[])
{
	const char * usage = { \
		"usage:\n" \
		"relay 0-2,4	turn on relay k0,k1,k2,k4\n" \
	};

	if (argc >= 2) {
		int relays = cmd_pattern_get(argv[1]);
		relays = rly_map(relays);
		mbi5025_write_and_latch(&rut_mbi, &relays, sizeof(relays));
		return 0;
	}

	printf(usage);
	return 0;
}
const cmd_t cmd_relay = {"relay", cmd_relay_func, "route board relay set cmds"};
DECLARE_SHELL_CMD(cmd_relay)
