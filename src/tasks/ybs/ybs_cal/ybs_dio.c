/*
 * 	miaofng@2013-3-27 initial version
 *	communication protocol for ybs
 *
 */

#include "ulp/sys.h"
#include "stm32f10x.h"
#include "uart.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "ybs_mon.h"
#include "ybs_dio.h"
#include <string.h>

#define __cmd(x) (ybs_version == 0x20) ? "ybs -"##x##"\n\r" : x
#define YBS_RESPONSE_MS 500

static struct ybs_mfg_data_s mfg_data;
static int ybs_version;

static void __ybs_init(void)
{
	ybs_uart_sel();
	if(ybs_version == 0 || ybs_version == 0x20) {
		uart_cfg_t cfg = { .baud = 9600 };
		ybs_uart.init(&cfg);
		uart_puts(&ybs_uart, "\rybs uuuuu\r\n");

		time_t deadline = time_get(YBS_RESPONSE_MS);
		while(1) {
			if(time_left(deadline) < 0) {
				sys_error("ybs response timeout");
				break;
			}

			if(ybs_uart.poll()) {
				int v = ybs_uart.getchar();
				ybs_version = (v == '0') ? 0x20 : ybs_version;
				break;
			}
		}
	}

	if(ybs_version == 0 || ybs_version == 0x10) {
		uart_cfg_t cfg = { .baud = 115200};
		ybs_uart.init(&cfg);
		//warnning: to unlock, ybs response is forbid!!!
		for(int i = 0; i < 5; i ++) ybs_uart.putchar('u');
		ybs_version = 0x10;
	}

	uart_flush(&ybs_uart);
}

static char cksum(const void *data, int n)
{
	const char *p = (const char *) data;
	char sum = 0;

	for(int i = 0; i < n; i ++) sum += p[i];
	return sum;
}

static int __ybs_mfg_read(void)
{
	uart_flush(&ybs_uart);
	uart_puts(&ybs_uart, __cmd("r"));

	int ecode = -E_YBS_TIMEOUT, i = 0;
	char *p = (char *) &mfg_data;
	time_t deadline = time_get(YBS_RESPONSE_MS);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}

		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		p[i ++] = (char) v;
		if(i >= sizeof(mfg_data)) {
			ecode = 0;
			//verify checksum
			if(cksum(&mfg_data, sizeof(mfg_data))) {
				ecode = -E_YBS_FRAME;
				sys_error("ybs mfg data cksum error");
			}
			break;
		}
	}
	return ecode;
}

static int __ybs_mfg_write(void)
{
	uart_flush(&ybs_uart);
	uart_puts(&ybs_uart, __cmd("w"));

	mfg_data.cksum = 0;
	mfg_data.cksum = -cksum(&mfg_data, sizeof(mfg_data));
	sys_mdelay(10);
	uart_send(&ybs_uart, &mfg_data, sizeof(mfg_data));

	int ecode = -E_YBS_TIMEOUT;
	time_t deadline = time_get(YBS_RESPONSE_MS);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}

		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		ecode = (v == '0') ? 0 : -E_YBS_RESPONSE;
		break;
	}
	return ecode;
}

/*SN: 20130328001, CAL: 8000-0000-8000-0000, SW: 18:09:22 Mar 15 2013\r*/
static int __ybs_info_decode(char *line, struct ybs_info_s *info)
{
	memset(info, 0x00, sizeof(struct ybs_info_s));

	//decode serial number
	line = strstr(line, "SN: ");
	if(line == NULL) return -E_YBS_FRAME;
	memcpy(info->sn, line + 4, 11);
	char *p = strchr(info->sn, ',');
	if(p != NULL) { //length of sn is smaller than 11
		*p = 0;
	}

	//decode hardware info
	if(strstr(line, "HW: ") == NULL) { //ybs v1.x do not has HW:
		//decode cal data
		line = strstr(line, "CAL: ");
		if(line == NULL) return -E_YBS_FRAME;
		int Gi, Di, Go, Do;
		sscanf(line + 5, "%x-%x-%x-%x", &Gi, &Di, &Go, &Do);
		Gi = (Gi > 0) ? Gi : -Gi;
		Go = (Go > 0) ? Go : -Go;
		info->Gi = Y2G(Gi);
		info->Go = Y2G(Go);
		info->Di = Y2D(Di);
		info->Do = Y2D(Do);
	}
	else { //try to get through command "ybs -r"
		sys_mdelay(10); //must add this line, to solve ghost 0x0a issue!!!
		int ecode = __ybs_mfg_read();
		if(ecode) return ecode;

		info->mfg_data = &mfg_data;
		info->Gi = mfg_data.Gi;
		info->Di = mfg_data.Di;
		info->Go = mfg_data.Go;
		info->Do = mfg_data.Do;
	}

	line = strstr(line, "SW: ");
	if(line == NULL) return -E_YBS_FRAME;
	line += 4;
	memset(info->sw, 0x00, 32);
	memcpy(info->sw, line, strlen(line) - 1);
	return 0;
}

int ybs_init(struct ybs_info_s *info)
{
	ybs_version = 0x00;
	__ybs_init();
	uart_puts(&ybs_uart, __cmd("i"));

	char *p = sys_malloc(128);
	sys_assert(p != NULL);
	memset(p, 0x00, 128);

	int ecode = -E_YBS_TIMEOUT, i = 0;
	time_t deadline = time_get(YBS_RESPONSE_MS);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}

		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		p[i ++] = (char) v;
		if((i == 127) || (v == '\r')) {
			//decode
			ecode = __ybs_info_decode(p, info);
			if(ecode) {
				sys_error("incorrect ybs response frame");
			}
			break;
		}
	}
	sys_free(p);
	return ecode;
}

int ybs_reset(void)
{
	__ybs_init();
	uart_puts(&ybs_uart, __cmd("k"));

	int ecode = -E_YBS_TIMEOUT;
	time_t deadline = time_get(3000);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}

		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		ecode = (v == '0') ? 0 : -E_YBS_RESPONSE;
		break;
	}
	return ecode;
}

int ybs_read(float *gf)
{
	__ybs_init();
	uart_puts(&ybs_uart, __cmd("f"));

	char p[2];
	int i = 0, ecode = -E_YBS_TIMEOUT;
	time_t deadline = time_get(YBS_RESPONSE_MS);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}

		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		p[i ++] = (char) v;
		if(i >= 2) {
			ecode = 0;
			short mgfx10;
			memcpy(&mgfx10, p, 2);
			*gf = mgfx10 / 100.0;
			break;
		}
	}
	return ecode;
}

int ybs_cal_read(float *gf)
{
	int ecode = ybs_cal_config(1.0, 0, 0, 0);
	if(ecode == 0) {
		ecode = ybs_read(gf);
	}
	return ecode;
}

int ybs_cal_write(float gf)
{
	int ecode = ybs_cal_config(0, 0, 0, gf);
	return ecode;
}

int ybs_cal_config(float fGi, float fDi, float fGo, float fDo)
{
	if(ybs_version == 0x10) {
		short Gi = G2Y(fGi);
		short Di = D2Y(fDi);
		short Go = G2Y(fGo);
		short Do = D2Y(fDo);
		__ybs_init();
		ybs_uart.putchar('c');
		char *p = (char *) &Gi;
		ybs_uart.putchar(p[0]);
		ybs_uart.putchar(p[1]);
		p = (char *) &Di;
		ybs_uart.putchar(p[0]);
		ybs_uart.putchar(p[1]);
		p = (char *) &Go;
		ybs_uart.putchar(p[0]);
		ybs_uart.putchar(p[1]);
		p = (char *) &Do;
		ybs_uart.putchar(p[0]);
		ybs_uart.putchar(p[1]);

		int ecode = -E_YBS_TIMEOUT;
		time_t deadline = time_get(YBS_RESPONSE_MS);
		while(1) {
			if(time_left(deadline) < 0) {
				sys_error("ybs response timeout");
				break;
			}
			if(ybs_uart.poll() == 0)
				continue;

			int v = ybs_uart.getchar();
			ecode = (v == '0') ? 0 : -E_YBS_RESPONSE;
			break;
		}
		return ecode;
	}

	mfg_data.Gi = fGi;
	mfg_data.Di = fDi;
	mfg_data.Go = fGo;
	mfg_data.Do = fDo;
	return __ybs_mfg_write();
}

int ybs_save(void)
{
	__ybs_init();
	uart_puts(&ybs_uart, __cmd("S"));

	int ecode = -E_YBS_TIMEOUT;
	time_t deadline = time_get(YBS_RESPONSE_MS);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("ybs response timeout");
			break;
		}
		if(ybs_uart.poll() == 0)
			continue;

		int v = ybs_uart.getchar();
		ecode = (v == '0') ? 0 : -E_YBS_RESPONSE;
		break;
	}
	return ecode;
}

static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ybs -i			ybs info read\n"
		"ybs -k			emulate reset key be pressed\n"
		"ybs -f [ms]		read gf(multiple read, ms per sample)\n"
		"ybs -r			read raw gf from ADC for calibration\n"
		"ybs -w gf		write raw gf to DAC for calibration\n"
		"ybs -S			command ybs to save current settings\n"
		"ybs -c	Gi Di Go Do	config ybs with calbration parameters(float)\n"
		"ybs -g	[baud]		exchange console to ybs, pls type 'q' to exit\n"
	};

	struct ybs_info_s info;
	int ms, e = 1, ecode, baud;
	float gf;
	char *line;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'i':
				ecode = ybs_init(&info);
				if(ecode == 0) {
					printf("SW: %s\n", info.sw);
					printf("SN: %s\n", info.sn);
					printf("Gi: %f\n", info.Gi);
					printf("Di: %f gf\n", info.Di);
					printf("Go: %f\n", info.Go);
					printf("Do: %f gf\n", info.Do);
				}
				else sys_error("ybs init fail");
				break;
			case 'k':
				ecode = ybs_reset();
				printf("reset %s(%d)\n", (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'f':
				ms = -1;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &ms) == 1)) {
					i ++;
				}
				do {
					ecode = ybs_read(&gf);
					printf("%f gf ... %s(%d)\n", gf, (ecode) ? "FAIL" : "PASS", ecode);
					sys_mdelay(ms);
				} while(ms > 0);
				break;
			case 'r':
				ecode = ybs_cal_read(&gf);
				printf("%f gf ... %s(%d)\n", gf, (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'w':
				gf = atof(argv[++ i]);
				ecode = ybs_cal_write(gf);
				printf("write %f gf ... %s(%d)\n", gf, (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'c':
				ecode = -1;
				if(argc - i > 4) {
					float Gi = atof(argv[++ i]);
					float Di = atof(argv[++ i]);
					float Go = atof(argv[++ i]);
					float Do = atof(argv[++ i]);
					ecode = ybs_cal_config(Gi, Di, Go, Do);
					printf("setting GiDiGoDo to %04x %04x %04x %04x ... %s(%d)\n", G2Y(Gi), D2Y(Di), G2Y(Go), D2Y(Do), (ecode) ? "FAIL" : "PASS", ecode);
				}
				break;
			case 'S':
				ecode = ybs_save();
				printf("save %s(%d)\n", (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'g':
				baud = (argc > 2) ? atoi(argv[2]) : 9600;
				{ //ybs communication i/f init
					uart_cfg_t cfg;
					cfg.baud = baud;
					ybs_uart.init(&cfg);
					monitor_init();
					ybs_uart_sel();
					printf("welcome to ybs debug console, type 'q' to exit\n");
					line = sys_malloc(128);
				}
				while(1) {
					//sys_update();
					while(ybs_uart.poll()) {
						char c = ybs_uart.getchar();
						putchar(c);
					}
					if(shell_ReadLine("ybs> ", line)) {
						if(line[0] == 'q') break;
						uart_send(&ybs_uart, line, strlen(line));
						uart_send(&ybs_uart, "\r\n", 2);
					}
				}
				sys_free(line);
				break;
			default:
				e ++;
			}
		}
	}

	if(e) {
		printf("%s", usage);
	}
	return 0;
}

const cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs digital i/f cmds"};
DECLARE_SHELL_CMD(cmd_ybs)
