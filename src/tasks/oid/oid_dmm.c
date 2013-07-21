/*
*	v2.0
*	dmm program based on aduc processor
*	1, do adc convert continuously
*	2, serv for spi communication
*
*	v2.1@2013.7
*	1, spi -> uart communication
*	2, p2.0 for open load detection
*
*/
#include "config.h"
#include "ulp/sys.h"
#include "oid_dmm.h"
#include "spi.h"
#include "aduc706x.h"
#include <string.h>
#include "aduc_adc.h"
#include "aduc_matrix.h"
#include "led.h"
#include "aduc706x.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "uart.h"

#define __DEBUG(X) if(debug_mode == 'D') {X}

#define CONFIG_RZ_OHM 10.0 //RZ, unit: ohm
#define dmm_uart_cnsl ((struct console_s *) &uart0)

static char debug_mode = 'A'; //Auto/Manual/Detail mode

static const aduc_adc_cfg_t cfg_mohm = {
	.adc0 = 1,
	.adc1 = 1,
	.pga0 = 0,
	.pga1 = 0,
	.mux0 = ADUC_MUX0_DCH01,
	.mux1 = ADUC_MUX1_DCH23,
};

static const aduc_adc_cfg_t cfg_o2mv = {
	.adc0 = 1,
	.adc1 = 0,
	.pga0 = 0,
	.mux0 = ADUC_MUX0_DCH01,
	.vofs = 1000,
};

static dmm_data_t dmm_data; /*current configuration & test result*/

/*force is used to ignore old setting*/
static int dmm_switch(const dmm_data_t *new, int force)
{
	int switched = 0;

	//to do run mode or channel switch?
	if(force || (new->pinA != dmm_data.pinA) || (new->pinK != dmm_data.pinK)) {
		matrix_pick(new->pinA, new->pinK);
		dmm_data.pinA = new->pinA;
		dmm_data.pinK = new->pinK;
		switched = 1;
	}

	if(force || (new->mohm != dmm_data.mohm)) {
		const aduc_adc_cfg_t *cfg = (new->mohm) ? &cfg_mohm : &cfg_o2mv;
		aduc_adc_init(cfg, ADUC_CAL_NONE);
		if(new->mohm) RELAY_R_MODE();
		else RELAY_V_MODE();
		dmm_data.mohm = new->mohm;
		switched = 1;
	}

	return switched;
}

static void dmm_init(void)
{
	dmm_data.value = 0;

	//console work mode
	shell_mute_set(dmm_uart_cnsl, (debug_mode == 'A') ? 1 : 0);

	//configure p2.0 as gpio input
	int m, v;
	v = GP2CON;
	m = 0x03;
	v &= ~m;
	v |= 0x0000;
	GP2CON = v;

	v = GP2DAT;
	m = 1 << 24;
	v &= ~m;
	GP2DAT = v; /*P20 = input*/

	//setting voltage condition for aduc adc calibration
	//aduc's bug?
	static const dmm_data_t dmm_data_cal = { .mohm = 1, .pinA = 3, .pinK = 4};
	dmm_switch(&dmm_data_cal, 1);
	sys_mdelay(500);

	static const aduc_adc_cfg_t cfg_cal = {
		.adc0 = 1,
		.adc1 = 1,
		.pga0 = 0,
		.pga1 = 0,
		.mux0 = ADUC_MUX0_DCH01,
		.mux1 = ADUC_MUX1_DCH23,
	};
	aduc_adc_init(&cfg_cal, ADUC_CAL_FULL);
}

static void dmm_update(void)
{
	if(!aduc_adc_is_ready(0))
		return;

	dmm_data.pass = 1;

	int e, d0, d1;
	e = aduc_adc_get(&d0, &d1);
	if(e) {
		__DEBUG(printf("\rerror                    ");)
		dmm_data.result = DMM_DATA_INVALID;
		if(dmm_data.mohm) {
			if((GP2DAT & 0x01) == 0x00)
				dmm_data.result = DMM_DATA_UNKNOWN;
		}
		return;
	}

	float mv0 = d0 * 1200.0 / (1 << 23);
	float mv1 = d1 * 1200.0 / (1 << 23);
	if(dmm_data.mohm) {
		float ohm = mv0 * CONFIG_RZ_OHM / mv1;
		__DEBUG(printf("mv0 = %.4f mv1 = %.4f ohm = %.4f\n", mv0, mv1, ohm);)
		dmm_data.result = (int) (ohm * 1000);
	}
	else {
		__DEBUG(printf("mv = %.3f\n", mv0);)
		dmm_data.result = (int) mv0;
	}
}

int main(void)
{
	sys_init();
        led_flash(LED_RED);
        led_flash(LED_GREEN);

	matrix_init();
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
		"dmm -A/M/D			dmm debug mode switch: Auto/Manual/Detailed\n"
		"dmm -p pinA pinK		switch dmm matrix to specified pin\n"
		"dmm -V				switch to voltage mode\n"
		"dmm -R				switch to resistor mode\n"
		"dmm -r				read current test result\n"
	};

	int e = 0, x, y, read = 0;
	dmm_data_t new;
	new.value = dmm_data.value;

	for(int j, i = 1; (i < argc) && (e == 0); i ++) {
		e += (argv[i][0] != '-');
		switch(argv[i][1]) {
		case 'A':
		case 'M':
		case 'D':
			debug_mode = argv[i][1];
			shell_mute_set(dmm_uart_cnsl, (debug_mode == 'A') ? 1 : 0);
			break;

		case 'p':
			e = 2;
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				x = atoi(argv[++ i]);
				e --;
			}
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				y = atoi(argv[++ i]);
				e --;
			}

			if(!e) {
				new.pinA = x;
				new.pinK = y;
			}
			break;

		case 'V':
			new.mohm = 0;
			break;
		case 'R':
			new.mohm = 1;
			break;

		case 'r':
			read = 1;
			break;

		default:
			e ++;
			break;
		}
	}

	if(!e) {
		if(dmm_switch(&new, 0)) {
			dmm_data.pass = 0;
		}
		if(read) {
			if(debug_mode == 'A') printf("%08x", dmm_data.value);
			else printf("%s%d%d = %d %s\n", \
				(dmm_data.mohm) ? "R" : "V", dmm_data.pinA, dmm_data.pinK, \
				dmm_data.result, (dmm_data.mohm) ? "mohm" : "mv", \
				(dmm_data.pass) ? "" : "(error???)");
		}
	}

	if(e && (debug_mode != 'A')) {
		printf("%s", usage);
	}

	printf("\n");
	return 0;
}

const cmd_t cmd_dmm = {"dmm", cmd_dmm_func, "dmm debug cmds"};
DECLARE_SHELL_CMD(cmd_dmm)
