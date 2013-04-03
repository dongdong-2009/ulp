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

#define YBS_RESPONSE_MS 100

static void __ybs_init(void)
{
	ybs_uart_sel();
	uart_cfg_t cfg = {
		.baud = 115200,
	};
	ybs_uart.init(&cfg);
	//warnning: to unlock, ybs response is forbid!!!
	for(int i = 0; i < 5; i ++) ybs_uart.putchar('u');
	uart_flush(&ybs_uart);
}

/*SN: 20130328001, CAL: 8000-0000-8000-0000, SW: 18:09:22 Mar 15 2013\r*/
static int __ybs_info_decode(char *line, struct ybs_info_s *info)
{
	//decode serial number
	line = strstr(line, "SN: ");
	if(line == NULL) return -E_YBS_FRAME;
	memset(info->sn, 0x00, 12);
	memcpy(info->sn, line + 4, 11);

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

	line = strstr(line, "SW: ");
	if(line == NULL) return -E_YBS_FRAME;
	line += 4;
	memset(info->sw, 0x00, 32);
	memcpy(info->sw, line, strlen(line) - 1);
	return 0;
}

int ybs_init(struct ybs_info_s *info)
{
	__ybs_init();
	ybs_uart.putchar('i');

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
	ybs_uart.putchar('k');

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

int ybs_read(int *mgf)
{
	__ybs_init();
	ybs_uart.putchar('f');

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
			*mgf = (int) mgfx10 * 10;
			break;
		}
	}
	return ecode;
}

int ybs_cal_read(int *mgf)
{
	int ecode = ybs_cal_config(1.0, 0, 0, 0);
	if(ecode == 0) {
		ecode = ybs_read(mgf);
	}
	return ecode;
}

int ybs_cal_write(int mgf)
{
	int ecode = ybs_cal_config(0, 0, 0, mgf);
	return ecode;
}

int ybs_cal_config(float fGi, float fDi, float fGo, float fDo)
{
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

int ybs_save(void)
{
	__ybs_init();
	ybs_uart.putchar('S');

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
		"ybs -f			read mgf10\n"
		"ybs -r			read raw mgf10 from ADC for calibration\n"
		"ybs -w mgf10		write raw mgf10 to DAC for calibration\n"
		"ybs -S			command ybs to save current settings\n"
		"ybs -c	Gi Di Go Do	config ybs with calbration parameters\n"
	};

	struct ybs_info_s info;
	int mgf, e = 1, ecode;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'i':
				ecode = ybs_init(&info);
				if(ecode == 0) {
					float Gi = info.Gi;
					float Di = info.Di;
					float Go = info.Go;
					float Do = info.Do;
					printf("SW: %s\n", info.sw);
					printf("SN: %s\n", info.sn);
					printf("Gi: %04x(%f)\n", (unsigned) G2Y(Gi) & 0xffff, Gi);
					printf("Di: %04x(%3.0f mgf)\n", (unsigned) D2Y(Di) & 0xffff, Di);
					printf("Go: %04x(%f)\n", (unsigned) G2Y(Go) & 0xffff, Go);
					printf("Do: %04x(%3.0f mgf)\n", (unsigned) D2Y(Do) & 0xffff, Do);
				}
				else printf("YBS FAIL\n");
				break;
			case 'k':
				ecode = ybs_reset();
				printf("reset %s(%d)\n", (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'f':
				ecode = ybs_read(&mgf);
				printf("%d mgf ... %s(%d)\n", mgf, (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'r':
				ecode = ybs_cal_read(&mgf);
				printf("%d mgf ... %s(%d)\n", mgf, (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'w':
				mgf = atoi(argv[++ i]);
				ecode = ybs_cal_write(mgf);
				printf("write %d mgf ... %s(%d)\n", mgf, (ecode) ? "FAIL" : "PASS", ecode);
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
