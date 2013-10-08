/*
 * 	miaofng@2013-2-21 initial version
 *	used for oid hardware version 2.0 board
 */

#include "ulp/sys.h"
#include "oid_mcd.h"
#include <stdlib.h>
#include "shell/cmd.h"
#include "linux/sort.h"
#include "uart.h"
#include "oid_dmm.h"
#include "ulp_time.h"
#include <string.h>
#include "common/ansi.h"

#define __DEBUG(X) X

enum {
	MCD_OK,
	MCD_E_PARA, /*input parameter error*/
	MCD_E_TIMEOUT, /*communication timeout*/
	MCD_E_STRANGE, /*communication data unexpected*/
	MCD_E_OP_TIMEOUT, /*DMM op timeout fail, such as read/.. timeout*/
	MCD_E_OP_FAIL, /*op fail*/
};

static uart_bus_t *mcd_bus = &uart2;

static int __mcd_transceive(const char *msg, int *echo)
{
	char i, str[16];

	/*to avoid aduc uart irq too frequently*/
	static time_t mcd_timer = 0;
	while((mcd_timer != 0) && (time_left(mcd_timer) > 0)) {
		sys_update();
	}
	mcd_timer = time_get(50);

	mcd_bus->putchar('\n');
        i = 0;
	while(mcd_bus->poll() > 0) {
		char c = mcd_bus->getchar();
		i ++;
		if(i > 16) {
			sys_assert(1 == 0); //uart data chaos?
			return - MCD_E_STRANGE;
		}
	}

	int n = strnlen(msg, 32);
	sys_assert(n < 30); //msg is too long??
	for(int j = 0; msg[j] != 0; j ++) {
		if(msg[j] == '\n') mcd_bus->putchar('\r');
		mcd_bus->putchar(msg[j]);
	}

	time_t deadline = time_get(50);
	memset(str, 0x00, 16);
	for(i = 0; i < 15;) {
		if(mcd_bus->poll()) {
			char c = mcd_bus->getchar();
			if((c == '\n') || (c == '\r')) {
				break;
			}

			str[i] = c;
			i ++;
		}

		if(time_left(deadline) < 0) {
			//sys_assert(1 == 0);
			return - MCD_E_TIMEOUT;
		}
	}


	//resolve echo bytes
	if(echo) {
		*echo = htoi(str);
	}
	return 0;
}

static int mcd_transceive(const char *msg, int *echo)
{
	int ecode;
	for(int i = 0; i < 5; i ++) {
		ecode = __mcd_transceive(msg, echo);
		if(!ecode)
			break;

		__DEBUG(printf(ANSI_FONT_MAGENTA);)
		__DEBUG(printf("%s() abnormal(ecode = %d)!!!\n", __FUNCTION__, ecode);)
		__DEBUG(printf(ANSI_FONT_DEF);)
	}
	return ecode;
}

int mcd_init(void)
{
	static const uart_cfg_t cfg = {.baud = 9600};
	mcd_bus->init(&cfg);
	int ecode, err = mcd_transceive("dmm -A\n", &ecode);
	return (err);
}

int mcd_mode(char bank)
{
	const char *msg = (bank == DMM_MODE_R) ? "dmm -R\n" : "dmm -V\n";
	mcd_transceive(msg, NULL);
	dmm_data_t echo;
	time_t deadline = time_get(500);
	while(time_left(deadline) > 0) {
		int err = mcd_transceive("dmm -r\n", &(echo.value));
		int expect = (bank == DMM_MODE_R) ? 1 : 0;
		if((!err) && (echo.mohm == expect)) {
			return 0;
		}
	}
	return -1;
}

static int __mcd_read(int *value)
{
	dmm_data_t echo;
	time_t deadline = time_get(500);
	while(time_left(deadline) > 0) {
		int err = mcd_transceive("dmm -r\n", &(echo.value));
		if((!err) && echo.ready) {
			if(1) {/*dirty solution, iar's bug???*/
				int x = echo.result;
				x <<= 8;
				x >>= 8;
				*value = x;
			}
			return 0;
		}
	}
	return -1;
}

int mcd_read(int *uv_or_mohm)
{
	int err = 0, *p = sys_malloc(sizeof(int) * 7);
	if(p == NULL) return -1;

	for(int i = 0; i < 7; i ++) {
		err = __mcd_read(p + i);
		if(err) break;
	}

	if(!err) {
		sort(p, 7, 4, NULL, NULL);
		*uv_or_mohm = (p[2] + p[3] + p[4]) / 3;
	}

	sys_free(p);
	return 0;
}

/*
1, please short each group of 4 signals first, such as R0V/R0I, R1V/R1I, R2V/R2I ...
2, then you will have 6 pins, 0..5,
3, call mcd_pick to measure the voltage/resistor of a diode from pin0(+) to pin1(-)
*/
int mcd_pick(int pinA, int pinK)
{
	char msg[16];
	sprintf(msg, "dmm -p %d %d\n", pinA, pinK);
	mcd_transceive(msg, NULL);
	dmm_data_t echo;
	time_t deadline = time_get(500);
	while(time_left(deadline) > 0) {
		int err = mcd_transceive("dmm -r\n", &(echo.value));
		if((!err) && (echo.pinA == pinA) && (echo.pinK == pinK)) {
			return 0;
		}
	}
	return -1;
}

int mcd_xread(int ch, int *mv)
{
	return -1;
}

static int cmd_mcd_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mcd -i			mcd init\n"
		"mcd -m	[R/V/S/O]	change test mode before read\n"
		"mcd -y	0..15		turn on relay s0..15 before read\n"
		"mcd -p 01		pick(pin0, pin1) before read\n"
		"mcd -r [100]		read [100] times\n"
		"mcd -x			unit: mv, gain=25 for ybs monitor use only\n"
	};

	if(argc > 0) {
		char mode = DMM_MODE_R;
		int r = 0, v, ch, e = 0;
		for(int j, i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'i':
				e = mcd_init();
				if(e) {
					printf("mcd init error\n");
					return 0;
				}
				break;
			case 'm':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					mode = (argv[++ i][0] == 'V') ? DMM_MODE_V : mode;
				}
				e = mcd_mode(mode);
				if(e) {
					printf("mcd mode change error\n");
					return 0;
				}
				break;

/*			case 'y':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					int relay = atoi(argv[++ i]);
					mcd_relay(1 << relay);
				}
				break;
*/
			case 'p':
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					int pin = atoi(argv[++ i]);
					pin %= 100;
					mcd_pick(pin/10, pin%10);
				}
				break;

			case 'r':
				r = 1;
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					r = atoi(argv[++ i]);
				}
				break;

			case 'x':
				ch = -1;
				j = i + 1;
				if((j < argc) && (argv[j][0] != '-')) {
					ch = atoi(argv[++ i]);
				}
				e = mcd_xread(ch, &v);
				if(!e) printf("%d mV\n", v);
				else printf("ecode = %d\n", e);
				break;

			default:
				e ++;
			}
		}

		if(e) {
			printf("%s", usage);
			return 0;
		}

		while(r > 0) {
			r --;
			e = mcd_read(&v);
			printf("read %s(val = %d)\n", (e == 0) ? "pass" : "fail", v);
		}
	}
	return 0;
}

const cmd_t cmd_mcd = {"mcd", cmd_mcd_func, "mcd debug cmds"};
DECLARE_SHELL_CMD(cmd_mcd)

#if 0
void main(void)
{
	sys_init();
	mdelay(1000);
	mcd_init();

	while(1) {
		sys_mdelay(5);
	}
}
#endif
