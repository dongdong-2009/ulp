/*
 * 	miaofng@2013-2-16 initial version
 *	used for oid hardware version 2.0 board
 *
 *	miaofng@2013-7-24
 *	totaly rewrite dueto unstable of old version
 *	identify mode should tell:
 *	1, o2s type
 *	2, pin sequence
 */

#include "ulp/sys.h"
#include "oid.h"
#include "shell/cmd.h"
#include "spi.h"
#include "common/vchip.h"
#include "common/tstep.h"
#include "oid_mcd.h"
#include <string.h>
#include "led.h"
#include "common/debounce.h"
#include "uart.h"
#include "shell/shell.h"
#include "common/ansi.h"

#define __DEBUG(X) if(debug_mode == 'D') {X}

#define CONFIG_RZ_OHM 10.0 //RZ, unit: ohm
#define oid_uart_cnsl ((struct console_s *) &uart1)
static char debug_mode = 'D'; //Auto/Manual/Detail mode

static struct oid_config_s oid_config;
static struct oid_result_s oid_result;

static struct oid_s {
	/*handshake signals*/
	unsigned test : 1; /*system is busy on test ...*/
	unsigned halt : 1; /*command the test system to halt the operation*/
	unsigned fail : 1; /*test abort dueto some failure, pls ref oid_error()*/

	/*intermediate test result*/
	int R[NR_OF_PINS][NR_OF_PINS]; //unit: mohm


	/*hot test*/
	int timeout_ms;
	time_t timer;
	int o2mv; //o2 sensor voltage output

	/*!!!identification result!!!*/
	int mohm; //typical resistance
	char pin2func[NR_OF_PINS]; /*such as: pin1 = gray*/
	char func2pin[NR_OF_PINS]; /*such as: pin_gray = 1*/
	char grounded; //'Y', 'N', '?', 'X'
} oid;

static void oid_error(int ecode)
{
	__DEBUG(printf(ANSI_FONT_RED);)
	switch(ecode) {
	case OID_E_SYS_DMM:
		__DEBUG(printf("ERROR: dmm system error(ecode = %d)\n", ecode);)
		break;
	case OID_E_SYS_DMM_DATA:
		__DEBUG(printf("ERROR: dmm data error, pls check relay & cable(ecode = %d)\n", ecode);)
		break;
	default:
		__DEBUG(printf("ERROR: ecode = %d\n", ecode);)
		break;
	}
	__DEBUG(printf(ANSI_FONT_DEF);)

	oid.fail = 1;
	int n = sizeof(oid_result.ecode) / sizeof(int);
	for(int i = 0; i <  n - 1; i ++) {
		oid_result.ecode[i] = oid_result.ecode[i + 1];
	}
	oid_result.ecode[2] = ecode;
}

static const struct o2s_s* o2s_get(int index)
{
	static const struct o2s_s o2s_list[] = {
		{.tcode = 0x1901, .lines = 1, .grounded = 'X', .mohm = 0, .min = 0, .max = 0},
		{.tcode = 0x2901, .lines = 2, .grounded = 'Y', .mohm = 0, .min = 0, .max = 0},
		{.tcode = 0x2902, .lines = 2, .grounded = 'N', .mohm = 0, .min = 0, .max = 0},

		{.tcode = 0x3901, .lines = 3, .grounded = 'X', .mohm = 2100, .min = 1100, .max = 2800},
		{.tcode = 0x3902, .lines = 3, .grounded = 'X', .mohm = 3500, .min = 2800, .max = 4500},
		{.tcode = 0x3903, .lines = 3, .grounded = 'X', .mohm = 6000, .min = 5000, .max = 7000},
		{.tcode = 0x3904, .lines = 3, .grounded = 'X', .mohm = 9000, .min = 8000, .max = 10000}, //???

		{.tcode = 0x4901, .lines = 4, .grounded = 'Y', .mohm = 2100, .min = 1100, .max = 2800},
		{.tcode = 0x4902, .lines = 4, .grounded = 'Y', .mohm = 3500, .min = 2800, .max = 4500},
		{.tcode = 0x4903, .lines = 4, .grounded = 'Y', .mohm = 6000, .min = 5000, .max = 7000},
		{.tcode = 0x4907, .lines = 4, .grounded = 'Y', .mohm = 9000, .min = 8000, .max = 10000}, //???
		{.tcode = 0x4904, .lines = 4, .grounded = 'N', .mohm = 2100, .min = 1100, .max = 2800},
		{.tcode = 0x4905, .lines = 4, .grounded = 'N', .mohm = 3500, .min = 2800, .max = 4500},
		{.tcode = 0x4906, .lines = 4, .grounded = 'N', .mohm = 6000, .min = 5000, .max = 7000},
		{.tcode = 0x4908, .lines = 4, .grounded = 'N', .mohm = 9000, .min = 8000, .max = 10000}, //???
	};

	if(index > sizeof(o2s_list) / sizeof(struct o2s_s))
		return NULL;
	return o2s_list + index;
}

static int o2s_search(int lines, int mohm, char grounded)
{
	const struct o2s_s *o2s;
	for(int i = 0; (o2s = o2s_get(i)) != NULL; i ++) {
		int match = (o2s->lines == lines) ? 1 : 0;
		match += (o2s->mohm == mohm) ? 1 : 0;
		match += (o2s->grounded == grounded) ? 1 : 0;
		if(match == 3)
			return o2s->tcode;
	}
	return 0;
}

#define is_short(mohm) (((mohm) > 0) && ((mohm) < oid_short_threshold))
#define is_open(mohm)
static int is_heatwire(int mohm)
{
	const struct o2s_s *o2sp;
	for(int i = 0; (o2sp = o2s_get(i)) != NULL; i ++) {
		if((mohm >= o2sp->min) && (mohm <= o2sp->max))
			return o2sp->mohm;
	}
	return 0;
}

/*search not assigned pins*/
static int oid_search_empty(char *pin0, char *pin1)
{
	int n = 0;
	for(char i = PIN_1; i < NR_OF_PINS; i ++) {
		if(oid.pin2func[i] == FUNC_NONE) {
			if((n == 0) && (pin0 != NULL)) *pin0 = i;
			if((n == 1) && (pin1 != NULL)) *pin1 = i;
			n ++;
		}
	}
	return n;
}

/*search pin which is short to shell, assigned pins will be ignored*/
static int oid_search_gray(char *pin)
{
	int n = 0;
	for(char i = PIN_1; i < NR_OF_PINS; i ++) {
		if(oid.pin2func[i] != FUNC_NONE)
			continue;

		if(is_short(oid.R[0][i])) {
			if((pin != NULL) && (n == 0))
				*pin = i;
			n ++;
		}
	}
	return n;
}

/*search heatwire pin pair, assigned pins will be ignored*/
static int oid_search_white(char *pin0, char *pin1, int *mohm)
{
	int n = 0;
	for(char i = PIN_1; i < NR_OF_PINS; i ++) {
		if(oid.pin2func[i] != FUNC_NONE)
			continue;

		for(char j = i + 1; j < NR_OF_PINS; j ++) {
			if(oid.pin2func[j] != FUNC_NONE)
				continue;

			int r = is_heatwire(oid.R[i][j]);
			if(r != 0) {
				if(n == 0) {
					if(pin0 != NULL) *pin0 = i;
					if(pin1 != NULL) *pin1 = j;
					if(mohm != NULL) *mohm = r;
				}
				n ++;
			}
		}
	}
	return n;
}

/* hot test will:
1, start key been pressed again => cancel hot test operation
2, timer will pressed => dynamic change max test duration
3, voltage over threshold => auto test finished
*/
static int oid_hot_test(char pinA, char pinK)
{
	int mv, mv_th = (oid_config.mode == 'I') ? oid_hot_mv_th_ident : oid_hot_mv_th_diag;
	oid.timeout_ms = (oid.timeout_ms == 0)? oid_hot_timeout_ms : oid.timeout_ms;
	oid.timer = time_get(0);

	struct debounce_s ov; /*over voltage*/
	debounce_init(&ov, 10, 0);

	int e = mcd_pick(pinA, pinK);
	e += mcd_mode(DMM_MODE_V);

	while(oid.timeout_ms + time_left(oid.timer) > 0) {
		e += mcd_read(&mv);
		if(e) {
			oid_error(OID_E_SYS_DMM);
			return 0;
		}

		__DEBUG(printf("%.3f : V%d%d = %d mV\n", - time_left(oid.timer) / 1000.0, pinA, pinK, mv);)

		oid.o2mv = (mv > 0) ? mv : -mv;
		if(debounce(&ov, (oid.o2mv > mv_th) ? 1 : 0)) {
			if(ov.on) {
				return mv;
			}
		}
	}
	oid_error(OID_E_O2S_VOLTAGE_LOST);
	return 0;
}

void oid_hot_set_ms(int ms)
{
	oid.timeout_ms = ms;
}

int oid_hot_get_ms(void)
{
	return oid.timeout_ms + time_left(oid.timer);
}

int oid_hot_get_mv(void)
{
	return oid.o2mv;
}

static void oid_1_ident(void)
{
	oid.grounded = 'X';
	oid.pin2func[PIN_1] = FUNC_BLACK;
	oid.func2pin[FUNC_BLACK] = PIN_1;
}

static void oid_2_ident(void)
{
	oid.grounded = '?';
	//to identify grounded or not
	char pin, pin_black, pin_gray;
	int mv, n = oid_search_gray(&pin);
	switch(n){
	case 0:
		if(oid_config.grounded == 'Y') {
			oid.grounded = 'Y';
			oid_error(OID_E_GROUNDED_PIN_LOST);
			return;
		}
		oid.grounded = 'N';
		oid_search_empty(&pin_gray, &pin_black);
		mv = oid_hot_test(pin_black, pin_gray);
		if(mv == 0) return; //hot test is been canceled
		pin = (mv > 0) ? pin_gray : pin_black;
		pin_black = (pin == pin_gray) ? pin_black : pin_gray;
		pin_gray = pin;
		break;
	case 1:
		if(oid_config.grounded == 'N') {
			oid.grounded = 'N';
			oid_error(OID_E_GROUNDED_PIN_TOO_MUCH);
			return;
		}
		oid.grounded = 'Y';
		oid_search_empty(&pin_gray, &pin_black);
		pin_black = (pin == pin_gray) ? pin_black : pin_gray;
		pin_gray = pin;
		break;
	default:
		oid_error(OID_E_GROUNDED_PIN_TOO_MUCH);
		return;
	}

	oid.pin2func[pin_gray] = FUNC_GRAY;
	oid.pin2func[pin_black] = FUNC_BLACK;
	oid.func2pin[FUNC_GRAY] = pin_gray;
	oid.func2pin[FUNC_BLACK] = pin_black;
}

/*black, white, white*/
static void oid_3_ident(void)
{
	oid.grounded = 'X';
	//to identify grounded or not
	char pin_white0, pin_white1, pin_black;
	int r, n = oid_search_white(&pin_white0, &pin_white1, &r);
	switch(n) {
	case 0:
		oid_error(OID_E_HEATWIRE_LOST);
		return;
	case 1:
		break;
	default:
		oid_error(OID_E_PIN_SHORT_TO_HEATWIRE);
		return;
	}

	oid.pin2func[pin_white0] = FUNC_WHITE0;
	oid.pin2func[pin_white1] = FUNC_WHITE1;
	oid.func2pin[FUNC_WHITE0] = pin_white0;
	oid.func2pin[FUNC_WHITE1] = pin_white1;
	oid.mohm = r;

	//the pin left must be black pin
	oid_search_empty(&pin_black, NULL);
	oid.pin2func[pin_black] = FUNC_BLACK;
	oid.func2pin[FUNC_BLACK] = pin_black;
}

static void oid_4_ident(void)
{
	oid.grounded = '?';
	//to identify grounded or not
	char pin, pin_white0, pin_white1, pin_gray, pin_black;
	int mv, r, n = oid_search_white(&pin_white0, &pin_white1, &r);
	switch(n) {
	case 0: //:(
		oid_error(OID_E_HEATWIRE_LOST);
		return;
	case 1: //:)
		break;
	default:
		oid_error(OID_E_PIN_SHORT_TO_HEATWIRE);
		return;
	}

	oid.pin2func[pin_white0] = FUNC_WHITE0;
	oid.pin2func[pin_white1] = FUNC_WHITE1;
	oid.func2pin[FUNC_WHITE0] = pin_white0;
	oid.func2pin[FUNC_WHITE1] = pin_white1;
	oid.mohm = r;

	n = oid_search_gray(&pin);
	switch(n){
	case 0:
		if(oid_config.grounded == 'Y') {
			oid.grounded = 'Y';
			oid_error(OID_E_GROUNDED_PIN_LOST);
			return;
		}
		oid.grounded = 'N';
		oid_search_empty(&pin_gray, &pin_black);
		mv = oid_hot_test(pin_black, pin_gray);
		if(mv == 0) return; //hot test is been canceled
		pin = (mv > 0) ? pin_gray : pin_black;
		pin_black = (pin == pin_gray) ? pin_black : pin_gray;
		pin_gray = pin;
		break;
	case 1:
		if(oid_config.grounded == 'N') {
			oid.grounded = 'N';
			oid_error(OID_E_GROUNDED_PIN_TOO_MUCH);
			return;
		}
		oid.grounded = 'Y';
		oid_search_empty(&pin_gray, &pin_black);
		pin_black = (pin == pin_gray) ? pin_black : pin_gray;
		pin_gray = pin;
		break;
	default:
		oid_error(OID_E_GROUNDED_PIN_TOO_MUCH);
		return;
	}

	oid.pin2func[pin_gray] = FUNC_GRAY;
	oid.pin2func[pin_black] = FUNC_BLACK;
	oid.func2pin[FUNC_GRAY] = pin_gray;
	oid.func2pin[FUNC_BLACK] = pin_black;
}

static void oid_identify(void)
{
	switch(oid_config.lines) {
	case 1:
		oid_1_ident();
		break;
	case 2:
		oid_2_ident();
		break;
	case 3:
		oid_3_ident();
		break;
	default:
		oid_4_ident();
		break;
	}

	int kcode = 0;
	for(int i = PIN_1; i < NR_OF_PINS; i ++) {
		kcode <<= 4;
		kcode |= oid.pin2func[i];
	}

	int tcode = o2s_search(oid_config.lines, oid.mohm, oid.grounded);
	oid_result.kcode = kcode;
	oid_result.tcode = tcode;
}

static void oid_1_diag(void)
{
}

static void oid_2_diag(void)
{
}

static void oid_3_diag(void)
{
}

static void oid_4_diag(void)
{
}

static void oid_diagnosis(void)
{
	switch(oid_config.lines) {
	case 1:
		oid_1_diag();
		break;
	case 2:
		oid_2_diag();
		break;
	case 3:
		oid_3_diag();
		break;
	default:
		oid_4_diag();
		break;
	}
}

void oid_start(const struct oid_config_s *cfg)
{
	memcpy(&oid_config, cfg, sizeof(struct oid_config_s));
	oid.halt = (oid.test) ? 1 : 0;  //auto clear by oid_update when test finish
	oid.test = 1; //auto clear by oid_update when test finish
}

void oid_get_result(struct oid_result_s *result)
{
	memcpy(result, &oid_result, sizeof(oid_result));
}

static void oid_update(void)
{
	if(! oid.test) return;
	//initialize glvar
	memset(&oid_result, 0x00, sizeof(oid_result));
	memset(&oid, 0x00, sizeof(oid));
	oid.test = 1;

	//mcd init&self cal
	mcd_init();

	//measure resistors
	int e = mcd_mode(DMM_MODE_R);
	if(e) {
		oid_error(OID_E_SYS_DMM);
		goto EXIT;
	}

	for(char pinA = PIN_0; pinA < NR_OF_PINS; pinA ++) {
		for(char pinK = pinA + 1; pinK < NR_OF_PINS; pinK ++) {
			int mohm = 0;
			e = mcd_pick(pinA, pinK);
			e += mcd_read(&mohm);
			if(e) {
				oid_error(OID_E_SYS_DMM);
				goto EXIT;
			}

			__DEBUG(printf("R[%d][%d] = %d mohm\n", pinA, pinK, mohm);)
			if((mohm != DMM_DATA_UNKNOWN) && (mohm != DMM_DATA_INVALID)) {
				e += (mohm < DMM_MOHM_MIN) ? 1 : 0;
				e += (mohm > DMM_MOHM_MAX) ? 1 : 0;
				if(e) {
					oid_error(OID_E_SYS_DMM_DATA);
					goto EXIT;
				}
			}

			oid.R[pinA][pinK] = mohm;
		}
	}

	if(oid_config.mode == 'I') oid_identify();
	else oid_diagnosis();

	__DEBUG(printf("TEST FINISH(kcode = %04x, tcode = %04x)!!!\n", oid_result.kcode, oid_result.tcode);)
EXIT:
	oid.test = 0;
	oid.halt = 0;
}

static void oid_init(void)
{
	memset(&oid, 0x00, sizeof(oid));
	oid_config.lines = 4;
	oid_config.mode = 'I';
	oid_config.grounded = '?';

	//console work mode
	shell_mute_set(oid_uart_cnsl, (debug_mode == 'A') ? 1 : 0);
}

void main(void)
{
	sys_init();
	oid_init();
	//oid_gui_init();
	led_on(LED_GREEN);
	led_flash(LED_RED);
	while(1) {
		sys_update();
		oid_update();
	}
}

void __sys_update(void)
{
	//oid_gui_update();
}

static int cmd_oid_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"oid -i	4 i y		oid start: [lines] [I/D] [Y/N/?]\n"
		"oid -A/M/D		oid debug mode: Auto/Manual/Detailed\n"
	};

	int e = 0;
	struct oid_config_s config;
	memcpy(&config, &oid_config, sizeof(config));

	for(int j, i = 1; (i < argc) && (e == 0); i ++) {
		e += (argv[i][0] != '-');
		switch(argv[i][1]) {
		case 'A':
		case 'M':
		case 'D':
			debug_mode = argv[i][1];
			shell_mute_set(oid_uart_cnsl, (debug_mode == 'A') ? 1 : 0);
			break;
		case 'i':
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				config.lines = atoi(argv[++ i]);
			}
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				config.mode = argv[++ i][0];
			}
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				config.grounded = argv[++ i][0];
			}
			oid_start(&config);
			break;
		default:
			e = -1;
			break;
		}
	}

	if(e && (debug_mode != 'A')) {
		printf("%s", usage);
	}
	return 0;
}

const cmd_t cmd_oid = {"oid", cmd_oid_func, "oid i/f cmds"};
DECLARE_SHELL_CMD(cmd_oid)
