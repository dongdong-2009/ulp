/*
*	ybs cal v2.0 miaofng@2014-2-10
*	port to calibration board v2.0(@lijun.xu)
*
*/
#include "config.h"
#include "ulp/sys.h"
#include "ybs_dmm.h"
#include "spi.h"
#include "aduc706x.h"
#include <string.h>
#include "aduc_adc.h"
#include "led.h"
#include "aduc706x.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "uart.h"
#include "common/ansi.h"

#define __DEBUG(X) //X
#define dmm_uart_cnsl ((struct console_s *) &uart0)

static struct dmm_result_s dmm_result;

void dmm_mux_init(void)
{
	int m, v;

	//configure p0.0/p0.1/p0.2 as gpio output
	m = 0x003f;
	v = GP0CON0;
	v &= ~m;
	GP0CON0 = v;

	m = 0x07 << 24;
	v = GP0DAT;
	v |= m;
	GP0DAT = v;
}

void dmm_mux_set(int ch)
{
	dmm_result.ready = 0;
	GP0CLR |= 0x07 << 16;
	GP0SET |= (1 << 16) << ch;
	sys_mdelay(100);
}

static void dmm_init(void)
{
	dmm_result.ready = 0;
	static const aduc_adc_cfg_t cfg = {
		.adc0 = 1,
		.adc1 = 0,
		.pga0 = 0,
		.pga1 = 0,
		.mux0 = ADUC_MUX0_DCH01,
		.mux1 = ADUC_MUX1_DCH23,
		.vofs = 300,
	};

	dmm_mux_init();
	aduc_adc_init(&cfg, ADUC_CAL_FULL);
}

static void dmm_update(void)
{
	if(aduc_adc_is_ready(0)) {
		int v, e = aduc_adc_get(&v, NULL);
		dmm_result.ovld = (e) ? 1 : 0;
		dmm_result.value = v * 1.2f / (1 << 23);
		dmm_result.ready = 1;
		__DEBUG(printf("%.3f v\n", dmm_result.value);)
	}
}

int main(void)
{
	sys_init();
        led_flash(LED_RED);
	shell_mute(dmm_uart_cnsl);

	dmm_init();
	while(1) {
		sys_update();
		dmm_update();
	}
}

static int cmd_dmm_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dmm -x ch		switch dmm to specified channel\n"
		"dmm -r			read current result\n"
	};

	int e = 0, v;
	for(int i = 1; (i < argc) && (e == 0); i ++) {
		e += (argv[i][0] != '-');
		switch(argv[i][1]) {
		case 'e':
			printf("ok\n\r");
			break;

		case 'x':
			e = -1;
			if(i + 1 < argc) {
				v = atoi(argv[++ i]);
				e = 0;
			}
			if(e) printf("1\n\r");
			else {
				printf("0\n\r");
				dmm_mux_set(v);
			}
			break;

		case 'r':
			if(!dmm_result.ready) printf("---- v\n\r");
			else if(dmm_result.ovld) printf("ovld v\n\r");
			else printf("%.3f v\n\r", dmm_result.value);
			break;

		default:
			printf("%s", usage);
			break;
		}
	}
	return 0;
}

const cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "dmm debug cmds"};
DECLARE_SHELL_CMD(cmd_dmm)
