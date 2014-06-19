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
#include "nvm.h"

#define __DEBUG(X) if(debug_mode == 'D') {X}

#define oid_uart_cnsl ((struct console_s *) &uart1)
static char debug_mode = 'D'; //Auto/Manual/Detail mode

static struct oid_config_s ocfg __nvm;
static struct {
	int val;
	int inv;
} oid_mohm_cal __nvm;

static struct o2s_config_s oid_config;
static struct oid_result_s oid_result;

static struct oid_s {
	/*handshake signals*/
	unsigned test : 1; /*system is busy on test ...*/
	unsigned halt : 1; /*command the test system to halt the operation*/
	unsigned fail : 1; /*test abort dueto some failure, pls ref oid_error()*/
	unsigned heating : 1; /*o2s is been heating*/

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
	char grounded; //'Y', 'N', '?', '-'
} oid;

static void oid_error(int ecode)
{
	__DEBUG(printf(ANSI_FONT_RED);)
	switch(ecode) {
	case OID_E_SYS_DMM_COMM:
		__DEBUG(printf("ERROR: dmm system error(ecode = %06x)\n", ecode);)
		break;
	case OID_E_SYS_DMM_DATA:
		__DEBUG(printf("ERROR: dmm data error, pls check relay & cable(ecode = %06x)\n", ecode);)
		break;
	default:
		__DEBUG(printf("ERROR: ecode = %06x\n", ecode);)
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
	if(index >= ocfg.nr_of_sensors)
		return NULL;
	return &(ocfg.o2s_list[index]);
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
#define is_open(mohm) ((mohm) == DMM_MOHM_OPEN)

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

note: pinA must be black pin in case of diag mode
*/
static int oid_hot_test(char pinA, char pinK)
{
	int mv, mv_th = (oid_config.mode == 'I') ? oid_hot_mv_th_ident : oid_hot_mv_th_diag;
	oid.timeout_ms = (oid.timeout_ms == 0)? oid_hot_timeout_ms : oid.timeout_ms;
	oid.timer = time_get(0);
	oid.heating = 1;

	struct debounce_s ov; /*over voltage*/
	debounce_init(&ov, 10, 0);

	int e = mcd_pick(pinA, pinK);
	e += mcd_mode(DMM_MODE_V);

	/*ignore halt commands before hot test to avoid mistake,
	halt command is introduced to halt hot test operation*/
	oid.halt = 0;

	while(oid.timeout_ms + time_left(oid.timer) > 0) {
		e += mcd_read(&mv);
		if(e) {
			oid_error(OID_E_SYS_DMM_COMM);
			oid.heating = 0;
			return 0;
		}

		__DEBUG(printf("%.3f : V%d%d = %d mV\n", - time_left(oid.timer) / 1000.0, pinA, pinK, mv);)

		oid.o2mv = (mv > 0) ? mv : -mv;
		if(debounce(&ov, (oid.o2mv > mv_th) ? 1 : 0)) {
			if(ov.on) {
				oid.heating = 0;
				if((oid_config.mode == 'D') && (mv < 0)) {
					oid_error(OID_E_O2S_VOLTAGE_POLAR);
				}
				return mv;
			}
		}

		if(oid.halt) {
			break;
		}
	}

	oid.heating = 0;
	oid_error(OID_E_O2S_VOLTAGE_LOST);
	return 0;
}

int oid_is_hot(void)
{
	return oid.heating;
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
	oid.grounded = '-';
	oid.mohm = 0;

	oid.pin2func[PIN_1] = FUNC_BLACK;
	oid.func2pin[FUNC_BLACK] = PIN_1;
}

static void oid_2_ident(void)
{
	oid.grounded = '?';
	oid.mohm = 0;

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
	oid.grounded = '-';
	oid.mohm = 0;

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
		oid_error(OID_E_HEATWIRE_MANY);
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
	oid.mohm = 0;

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
		oid_error(OID_E_HEATWIRE_MANY);
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
	//measure resistors
	int e = mcd_mode(DMM_MODE_R);
	if(e) {
		oid_error(OID_E_SYS_DMM_COMM);
		return;
	}

	for(char pinA = PIN_0; pinA < oid_config.lines + 1; pinA ++) {
		for(char pinK = pinA + 1; pinK < oid_config.lines + 1; pinK ++) {
			int mohm = 0;
			e = mcd_pick(pinA, pinK);
			e += mcd_read(&mohm);
			if(e) {
				oid_error(OID_E_SYS_DMM_COMM);
				return;
			}

			//__DEBUG(printf("R[%d][%d] = %d mohm\n", pinA, pinK, mohm);)
			if((mohm != DMM_DATA_UNKNOWN) && (mohm != DMM_DATA_INVALID)) {
				e += (mohm < DMM_MOHM_MIN) ? 1 : 0;
				e += (mohm > DMM_MOHM_MAX) ? 1 : 0;
				if(e) {
					oid_error(OID_E_SYS_DMM_DATA);
					return;
				}

				mohm -= oid_mohm_cal.val;
				mohm = (mohm < 0) ? 0 : mohm;
			}

			e = 0;
			e += is_short(mohm);
			e += is_open(mohm);
			e += is_heatwire(mohm);
			if(e == 0) {
				int ecode = (2 << 16)|(pinA << 12) | (pinK << 8) | 3;
				oid_error(ecode);
			}

			oid.R[pinA][pinK] = mohm;
			__DEBUG(printf("RCAL[%d][%d] = %d mohm\n", pinA, pinK, mohm);)
		}
	}

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

enum {SHORT = 1, OPEN = 2, HIGHR = 3, GRAY, HEATWIRE};
int oid_ohm_test(char pin0, char pin1, int expect)
{
	//swap if needed
	if(pin0 > pin1) {
		int pin = pin0;
		pin0 = pin1;
		pin1 = pin;
	}

	int ret, mohm = oid.R[pin0][pin1];
	int status = HIGHR;
	status = (is_short(mohm)) ? SHORT : status;
	status = (is_open(mohm)) ? OPEN : status;
	int ecode = (1 << 16)|(pin0 << 12) | (pin1 << 8) | status;

	switch(expect) {
	case OPEN:
	case SHORT:
		if(expect != status) {
			oid_error(ecode);
		}
		ret = status;
		break;
	case GRAY:
		expect = (oid_config.grounded == 'Y') ? SHORT : expect;
		expect = (oid_config.grounded == 'N') ? OPEN : expect;
		if((status == HIGHR) || ((expect != GRAY) && (expect != status))) {
			oid_error(ecode);
		}
		ret = (expect == GRAY) ? status : expect;
		break;
	case HEATWIRE:
		mohm = is_heatwire(mohm);
		if(mohm == 0) {
			oid_error(ecode);
		}
		ret = mohm;
		break;
	default:
		sys_assert(1 == 0);
	}

	return ret;
}

/*black*/
static void oid_1_diag(void)
{
	oid_ohm_test(PIN_BLACK, PIN_SHELL, OPEN);
	oid_hot_test(PIN_BLACK, PIN_SHELL);

	oid.grounded = '-';
	oid.mohm = 0;
}

/*gray, black*/
static void oid_2_diag(void)
{
	int status = oid_ohm_test(PIN_SHELL, PIN_GRAY, GRAY);
	oid_ohm_test(PIN_SHELL, PIN_BLACK, OPEN);
	oid_ohm_test(PIN_GRAY, PIN_BLACK, OPEN);
	oid_hot_test(PIN_BLACK, PIN_GRAY);

	oid.grounded = (status == SHORT) ? 'Y' : 'N';
	oid.mohm = 0;
}

/*black, white, white*/
static void oid_3_diag(void)
{
	oid_ohm_test(PIN_SHELL, PIN_BLACK, OPEN);
	oid_ohm_test(PIN_SHELL, PIN_WHITE0, OPEN);
	oid_ohm_test(PIN_SHELL, PIN_WHITE1, OPEN);
	oid_ohm_test(PIN_BLACK, PIN_WHITE0, OPEN);
	oid_ohm_test(PIN_BLACK, PIN_WHITE1, OPEN);
	int mohm = oid_ohm_test(PIN_WHITE0, PIN_WHITE1, HEATWIRE);
	oid_hot_test(PIN_BLACK, PIN_SHELL);

	oid.grounded = '-';
	oid.mohm = mohm;
}

/*gray, black, white, white*/
static void oid_4_diag(void)
{
	int status = oid_ohm_test(PIN_SHELL, PIN_GRAY, GRAY);
	oid_ohm_test(PIN_SHELL, PIN_BLACK, OPEN);
	oid_ohm_test(PIN_SHELL, PIN_WHITE0, OPEN);
	oid_ohm_test(PIN_SHELL, PIN_WHITE1, OPEN);
	oid_ohm_test(PIN_GRAY, PIN_BLACK, OPEN);
	oid_ohm_test(PIN_GRAY, PIN_WHITE0, OPEN);
	oid_ohm_test(PIN_GRAY, PIN_WHITE1, OPEN);
	oid_ohm_test(PIN_BLACK, PIN_WHITE0, OPEN);
	oid_ohm_test(PIN_BLACK, PIN_WHITE1, OPEN);
	int mohm = oid_ohm_test(PIN_WHITE0, PIN_WHITE1, HEATWIRE);
	oid_hot_test(PIN_BLACK, PIN_GRAY);

	oid.grounded = (status == SHORT) ? 'Y' : 'N';
	oid.mohm = mohm;
}

static void oid_diagnosis(void)
{
	memset(oid.R, 0x00, sizeof(oid.R));

	//measure resistors
	int e = mcd_mode(DMM_MODE_R);
	if(e) {
		oid_error(OID_E_SYS_DMM_COMM);
		return;
	}

	const unsigned char pin_mask[NR_OF_PINS][NR_OF_PINS] = {
		/*SHELL	GRAY	BLACK	WHITE0	WHITE1*/
		{0,	0,	0,	0,	0},
		{1,	0,	1,	0,	0}, //1 LINES
		{1,	1,	1,	0,	0}, //2 LINES
		{1,	0,	1,	1,	1}, //3 LINES
		{1,	1,	1,	1,	1}, //4 LINES
	};

	for(char pinA = PIN_0; pinA < NR_OF_PINS; pinA ++) {
		//skip not exist pins
		if(pin_mask[oid_config.lines][pinA] == 0)
			continue;

		for(char pinK = pinA + 1; pinK < NR_OF_PINS; pinK ++) {
			//skip not exist pins
			if(pin_mask[oid_config.lines][pinK] == 0)
				continue;

			int mohm = 0;
			e = mcd_pick(pinA, pinK);
			e += mcd_read(&mohm);
			if(e) {
				oid_error(OID_E_SYS_DMM_COMM);
				return;
			}

			//__DEBUG(printf("R[%d][%d] = %d mohm\n", pinA, pinK, mohm);)
			if((mohm != DMM_DATA_UNKNOWN) && (mohm != DMM_DATA_INVALID)) {
				e += (mohm < DMM_MOHM_MIN) ? 1 : 0;
				e += (mohm > DMM_MOHM_MAX) ? 1 : 0;
				if(e) {
					oid_error(OID_E_SYS_DMM_DATA);
					return;
				}

				mohm -= oid_mohm_cal.val;
				mohm = (mohm < 0) ? 0 : mohm;
			}

			oid.R[pinA][pinK] = mohm;
			__DEBUG(printf("RCAL[%d][%d] = %d mohm\n", pinA, pinK, mohm);)
		}
	}

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

	int kcode = 0;
	kcode |= oid_config.lines;
	kcode <<= 4;
	kcode |= (oid.grounded == 'Y') ? 1 : 0;
	kcode <<= 4;
	kcode <<= 4;

	if(oid.mohm) {
		int mohm = oid.R[PIN_WHITE0][PIN_WHITE1];
		int _kcode = (mohm / 1000) % 10;
		_kcode <<= 4;
		_kcode |= (mohm / 100) % 10;
		kcode |= _kcode;
	}

	int tcode = o2s_search(oid_config.lines, oid.mohm, oid.grounded);
	oid_result.kcode = kcode;
	oid_result.tcode = tcode;
}

void oid_start(void)
{
	if(oid.test) oid.halt = 1;
	else oid.test = 1; //auto clear by oid_update when test finish
}

int oid_is_busy(void)
{
	return oid.test;
}

void oid_set_config(const struct o2s_config_s *cfg)
{
	if(oid.test == 0) {
		memcpy(&oid_config, cfg, sizeof(struct o2s_config_s));
	}
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

	int e, mohm;

	//ocfg is correct?
	char sum = 0, *p = (char *) &ocfg;
	for(int i = 0; i < sizeof(ocfg); i ++) {
		sum += p[i];
	}
	if((ocfg.nr_of_sensors <= 0) || (sum != 0)) {
		oid_error(OID_E_SYS_CFG_ERROR);
		goto EXIT;
	}

	//mcd init&self cal
	e = mcd_init();
	if(e) {
		oid_error(OID_E_SYS_DMM_COMM);
		goto EXIT;
	}

	//self cal
#if 1
	mohm = 0;
	e += mcd_mode(DMM_MODE_R);
	e += mcd_pick(PIN_4, PIN_5);
	e += mcd_read(&mohm);
	if(e) {
		oid_error(OID_E_SYS_DMM_COMM);
		goto EXIT;
	}
	mohm -= oid_rcal_mohm;
	mohm = (mohm < 0) ? - mohm : mohm;
	if(mohm > oid_rcal_mohm_delta_max) {
		oid_error(OID_E_SYS_CAL);
		goto EXIT;
	}
#endif

	if(oid_config.mode == 'I') oid_identify();
	else oid_diagnosis();

	__DEBUG(printf("TEST FINISH(kcode = %04x, tcode = %04x)!!!\n", oid_result.kcode, oid_result.tcode);)
EXIT:
	oid.test = 0;
	oid.halt = 0;
}

static void oid_init(void)
{
	if(oid_mohm_cal.val != (~ oid_mohm_cal.inv)) {
		oid_mohm_cal.val = 0;
	}

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
#if CONFIG_TASK_GUI == 1
	oid_gui_init();
#endif
	led_on(LED_GREEN);
	led_flash(LED_RED);
	while(1) {
		sys_update();
		oid_update();
	}
}

void __sys_update(void)
{
#if CONFIG_TASK_GUI == 1
	oid_gui_update();
#endif
}

/* procedure:
1, pc send command "oid -A\n" to switch to auto mode
2, pc send command "oid -d bytes\n" wait for target echo
3, target enter into uart direct mode
4, target wait for bytes no more than 1s
5, target receive all bytes
6, target do cksum check
7, target echo "ACK" or "NAK"
8, target save the data received to flash
9, target goto normal mode
*/
int cmd_oid_config_dnload(int bytes)
{
	time_t deadline = time_get(3000);
	struct oid_config_s *cfg = sys_malloc(bytes);
	sys_assert(cfg != NULL);

	int i;
	char *p = (char *) cfg;
	for(i = 0; i < bytes;) {
		if(uart1.poll() > 0) {
			*p ++ = uart1.getchar();
			deadline = time_get(500);
                        i ++;
		}
		if(time_left(deadline) < 0) {
			break;
		}
	}

	if(i != bytes) {
		uart_puts(&uart1, "TIMEOUT");
		sys_free(cfg);
		return -1;
	}

	char sum = 0;
	p = (char *) cfg;
	for(i = 0; i < bytes; i ++) {
		sum += p[i];
	}
	if(sum != 0) {
		uart_puts(&uart1, "CKSUM");
		sys_free(cfg);
		return -2;
	}

	memset(&ocfg, 0x00, sizeof(ocfg));
	memcpy(&ocfg, cfg, bytes);
	sys_free(cfg);
	uart_puts(&uart1, "ACK");
	nvm_save();
	return 0;
}

/* procedure:
1, pc send command "oid -A\n" to switch to auto mode
2, pc send command "oid -u\n" wait for target upload data
3, target enter into uart direct mode
4, target send config data to pc
*/
int cmd_oid_config_upload(void)
{
	//ocfg is correct?
	char sum = 0, *p = (char *) &ocfg;
	for(int i = 0; i < sizeof(ocfg); i ++) {
		sum += p[i];
	}
	if((ocfg.nr_of_sensors <= 0) || (sum != 0)) {
		printf("CFG DATA IS INVALID");
		return -1;
	}

	printf("ACK");
	int bytes = sizeof(ocfg) - (32 - ocfg.nr_of_sensors) * sizeof(struct o2s_s);
	for(int i = 0; i < bytes; i ++) {
		uart1.putchar(p[i]);
	}
	return 0;
}

int cmd_oid_config_cal(void)
{
	printf(" ");

	int e, mohm;

	//mcd init&self cal
	e = mcd_init();
	if(!e) {
		mohm = 0;
		e += mcd_mode(DMM_MODE_R);
		e += mcd_pick(PIN_3, PIN_4);
		e += mcd_read(&mohm);
	}

	if(e) {
		printf("EMCD");
		return -1;
	}

	if(mohm > 800) {
		printf("EOPEN?");
		return -2;
	}

	oid_mohm_cal.val = mohm;
	oid_mohm_cal.inv = ~ mohm;
	nvm_save();
	printf("ACK%d", mohm);
	return 0;
}

static int cmd_oid_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"oid -i	4 I y		oid start or halt: [lines] [I/D] [Y/N/?]\n"
		"oid -A/M/D		oid debug mode: Auto/Manual/Detailed\n"
		"oid -d bytes		oid config data dnload\n"
		"oid -u			oid config data upload\n"
	};

	int n, e = 0;
	struct o2s_config_s config;
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
			oid_start();
			break;
		case 'd':
			if(((j = i + 1) < argc) && (argv[j][0] != '-')) {
				n = atoi(argv[++ i]);
				if((n > 0) && (n <= sizeof(ocfg))) {
					printf("ACK");
					cmd_oid_config_dnload(n);
					break;
				}
			}
			printf("NAK");
			break;
		case 'c':
			cmd_oid_config_cal();
			break;
		case 'u':
			cmd_oid_config_upload();
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
