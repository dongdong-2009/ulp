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
#include "bsp.h"

static const mbi5025_t rut_mbi = {.bus = &spi2, .load_pin = SPI_2_NSS};

/*to conver relay name order(LSn) to mbi5024 control line nr(Km)*/
static int rly_map(int relays)
{
	int image = 0;
	#define A(a) a
	#define B(b) b+16
	#define NC 31
	const char map[] = { \
		/*LS00-LS07*/ A( 0), A(12), A(10), A( 3), B( 5), B( 2), B( 4), NC,     \
		/*LS08-LS15*/ NC,    NC,    NC,    NC,    A( 2), A(14), A( 9), A( 4),  \
		/*LS15-LS23*/ B( 0), A( 6), A( 5), A( 7), NC,    A(11), A(15), B( 3),  \
		/*LS24-LS31*/ A( 1), A(13), B( 1), A( 8), NC,    NC,    NC,    NC      \
	};

	for(int i = 0; i < sizeof(map); i ++) {
		if(relays & (1 << i)) {
			int K = map[i];
			if(K != NC) {
				image |= 1 << K;
			}
		}
	}
	return image;
}

void rut_init(void)
{
	oe_set(1);
	mbi5025_Init(&rut_mbi);
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
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(6), /*RPB*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(4), /*RMX*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(5), /*RX2*/
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
	oe_set(0);
}

static int cmd_relay_func(int argc, char *argv[])
{
	const char * usage = { \
		"usage:\n" \
		"relay 0-2,4	turn on relay k0,k1,k2,k4\n" \
	};

	if ((argc >= 2) &&(strcmp(argv[1], "help"))) {
		int relays = cmd_pattern_get(argv[1]);
		relays = rly_map(relays);
		mbi5025_write_and_latch(&rut_mbi, &relays, sizeof(relays));
		oe_set(0);
		return 0;
	}

	printf(usage);
	return 0;
}
const cmd_t cmd_relay = {"relay", cmd_relay_func, "route board relay set cmds"};
DECLARE_SHELL_CMD(cmd_relay)
