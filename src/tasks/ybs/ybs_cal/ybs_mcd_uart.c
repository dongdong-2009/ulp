/*
 * 	miaofng@2013-2-21 initial version
 *	used for oid hardware version 2.0 board
 */

#include "ulp/sys.h"
#include "ybs_mcd.h"
#include <stdlib.h>
#include "shell/cmd.h"
#include "linux/sort.h"
#include "uart.h"
#include "dmm2/ybs_dmm.h"
#include "ulp_time.h"
#include <string.h>
#include "common/ansi.h"

#define __DEBUG(X) X
#define MCD_LINEBUF_BYTES 32
#define MCD_GAIN 25 //max5490 is 25:1

enum {
	MCD_OK,
	MCD_E_PARA, /*input parameter error*/
	MCD_E_TIMEOUT, /*communication timeout*/
	MCD_E_STRANGE, /*communication data unexpected*/
	MCD_E_OP_TIMEOUT, /*DMM op timeout fail, such as read/.. timeout*/
	MCD_E_OP_FAIL, /*op fail*/
};

static uart_bus_t *mcd_bus = &uart3;

static int __mcd_transceive(const char *msg, char *echo)
{
	char i, str[MCD_LINEBUF_BYTES];

	/*to avoid aduc uart irq too frequently*/
	static time_t mcd_timer = 0;
	while((mcd_timer != 0) && (time_left(mcd_timer) > 0)) {
		sys_update();
	}
	mcd_timer = time_get(50);

	//flush
	mcd_bus->putchar('\n');
        i = 0;
	while(mcd_bus->poll() > 0) {
		char c = mcd_bus->getchar();
		i ++;
		if(i > 64) {
			sys_assert(1 == 0); //uart data chaos?
			return - MCD_E_STRANGE;
		}
	}

	//send
	int n = strnlen(msg, 32);
	sys_assert(n < 30); //msg is too long??
	for(int j = 0; msg[j] != 0; j ++) {
		if(msg[j] == '\n') mcd_bus->putchar('\r');
		mcd_bus->putchar(msg[j]);
	}

	//recv
	time_t deadline = time_get(50);
	memset(str, 0x00, MCD_LINEBUF_BYTES);
	for(i = 0;;) {
		if(mcd_bus->poll()) {
			char c = mcd_bus->getchar();
			if((c == '\n') || (c == '\r')) {
                                str[i] = 0;
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
	int ecode = (echo != NULL) ? 0 : atoi(str);
	if(echo) {
		strcpy(echo, str);
	}
	return ecode;
}

static int mcd_transceive(const char *msg, char *echo)
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
	char str[MCD_LINEBUF_BYTES];
	static const uart_cfg_t cfg = {.baud = 9600};
	mcd_bus->init(&cfg);
	int ecode = mcd_transceive("dmm -e\n", str);
	ecode = (ecode) ? ecode : strcmp(str, "ok");
	return (ecode);
}

int mcd_mode(char bank)
{
	return -1; //not supported
}

/*success return 0*/
int mcd_switch(int ch)
{
	char msg[MCD_LINEBUF_BYTES], echo[MCD_LINEBUF_BYTES];

	sprintf(msg, "dmm -x %d\n", ch);
	int ecode = mcd_transceive(msg, echo);
	if(! ecode) {
		ecode = atoi(echo);
	}
	return ecode;
}

int mcd_read(int *mv)
{
	char response[MCD_LINEBUF_BYTES];
	int ecode = mcd_transceive("dmm -r\n", response);
	if(!ecode) {
		float v = 0.0;
		int n;

		ecode = - MCD_E_STRANGE;
		n = sscanf(response, "%f", &v);
		if(n == 1) {
			*mv = (int) (v * 1000 * MCD_GAIN);
			ecode = 0;
		}
	}
	return (ecode);
}

int mcd_xread(int ch, int *mv)
{
	static int mcd_ch = -1;
	if(ch != mcd_ch) {
		mcd_ch = ch;
		int ecode = mcd_switch(mcd_ch);
		if(ecode)
			return ecode;
	}

	int samples[7], i;
	time_t deadline = time_get(3000);
	for(i = 0; time_left(deadline) > 0;) {
		if(!mcd_read(&samples[i])) {
			i ++;
			if(i == 7) {
				sort(samples, 7, sizeof(int), NULL, NULL);
				*mv = (samples[2] + samples[3] + samples[4]) / 3;
				return 0;
			}
		}
	}
	return - MCD_E_TIMEOUT;
}

static int cmd_mcd_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mcd -i			mcd init\n"
		"mcd -r [100]		read [100] times\n"
		"mcd -x	ch		ch: ASIG=0, DET=1, BURN=2, unit: mv\n"
	};

	if(argc > 0) {
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
	mdelay(500);
	static const uart_cfg_t cfg = {.baud = 9600};
	mcd_bus->init(&cfg);

	while(1) {
		sys_update();
		while(mcd_bus->poll()) {
			char c = mcd_bus->getchar();
			putchar(c);
		}
	}
}

static int cmd_dmm_func(int argc, char *argv[])
{
	for(int i = 0; i < argc; i ++) {
		uart_puts(mcd_bus, argv[i]);
		mcd_bus->putchar(' ');
	}
	mcd_bus->putchar('\n');
	mcd_bus->putchar('\r');
	return 0;
}

const cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "dmm debug console cmds"};
DECLARE_SHELL_CMD(cmd_dmm)
#endif
