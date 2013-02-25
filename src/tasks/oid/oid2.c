/*
 * 	miaofng@2013-2-16 initial version
 *	used for oid hardware version 2.0 board
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

struct oid_s oid;

static char oid_pin_search(char oid_pin)
{
	if(oid_pin > 0) { //mapped to o2s pin nr
		for(char i = 1; i < NR_OF_PINS; i ++) {
			if(oid.o2s[i] == oid_pin)
				return i;
		}
		return 0;
	}
	else { //to find an unused pin
		for(char i = 1; i <= oid.lines; i ++) {
			if(oid_pin_search(i) == 0)
				return i;
		}
		return 0;
	}
}

static void oid_error(int ecode)
{
	//sys_assert(ecode != E_SYSTEM);
	oid.ecode[0] = oid.ecode[1];
	oid.ecode[1] = oid.ecode[2];
	oid.ecode[2] = ecode;
}

//configure oid/gui default value
static void oid_init(void)
{
	memset(&oid, 0x00, sizeof(oid));
	oid.lines = 4;
	oid.mode = 'i';
	oid.gnd = '?';
	oid.seconds = oid_sec_threshold;
}

static int oid_start(void)
{
	sys_update();
	int ecode = 1;
	if(oid.start) {
		ecode = 0;
		oid.start = 0;
		oid.lock = 1;
		oid.scnt = 0;
		oid.ecode[0] = oid.ecode[1] = oid.ecode[2] = 0;
		oid.tcode = oid.kcode = 0;
		oid.mohm_real = oid.mohm = oid.mv = 0;
		oid.gnd = gui.gnd;
	}

	return ecode;
}

static int oid_self_check(void)
{
	int mohm, ecode = mcd_init();
	if(ecode == 0) {
		mcd_mode(DMM_R_AUTO);
		mcd_pick(4, 5);
		ecode = mcd_read(&mohm);
		printf("RZ = %d mohm\n", mohm);
		mohm -= 10000;
		mohm = (mohm > 0) ? mohm : -mohm;
		if(mohm < 100) {
			return 0;
		}
	}

	oid_error(E_SYSTEM);
	return 1;
}

static int oid_measure_resistors(void)
{
	int ecode = 0, mohm;
	mcd_mode(DMM_R_AUTO);
	for(int px = 0; px <= oid.lines; px ++) {
		for(int py = px + 1; py <= oid.lines; py ++) {
			mcd_pick(px, py);
			ecode += mcd_read(&mohm);
			oid.mohms[px][py] = mohm;

			/*
			if(mohm < 1000 * 1000) { //< 1KOHM
				mcd_mode(DMM_R_AUTO);
				ecode += mcd_read(&mohm);
				oid.mohms[px][py] = mohm;
				mcd_mode(DMM_R_OPEN);
			}
			*/
			printf("R[%d][%d] = %d mohm\n", px, py, mohm);
		}
	}

	if(ecode > 0) {
		oid_error(E_SYSTEM | ecode);
		return 1;
	}
	return 0;
}

/*!!! warnning: the oxgen sensor must be good, or identify result may be incorrect. This
routine will try its best to obtain more info even if failture events are detected.
input: oid.mohms[][]
output: oid.o2s[xxx], oid.gnd, oid.mohm, oid.tcode
*/
static int oid_identify(void)
{
	int n_short_gnd = 0; /*short & pinx == 0*/
	int n_short_other = 0; /*short, except pinx == 0*/
	int n_wire = 0;
	int n_rstrange = 0;

	memset(oid.o2s, 0x00, sizeof(oid.o2s));

	//scan the mohms[][] to get statistics info
	for(int px = 0; px <= oid.lines; px ++) {
		for(int py = px + 1; py <= oid.lines; py ++) {
			int mohm = oid.mohms[px][py];
			printf("R[%d][%d] = %d mohm\n", px, py, mohm);
			if(mohm < mohm_short_threshold) {
				if(px == 0) {
					n_short_gnd ++;
					oid.o2s[PIN_GRAY] = py;
				}
				else {
					n_short_other ++;
				}
			}
			else if((mohm > 1100) && (mohm < 2800)) {
				oid.mohm = 2100;
				oid.mohm_real = mohm;
				oid.o2s[PIN_WHITE0] = px;
				oid.o2s[PIN_WHITE1] = py;
				n_wire ++;
			}
			else if((mohm > 2800) && (mohm < 4500)) {
				oid.mohm = 3500;
				oid.mohm_real = mohm;
				oid.o2s[PIN_WHITE0] = px;
				oid.o2s[PIN_WHITE1] = py;
				n_wire ++;
			}
			else if((mohm > 5000) && (mohm < 7000)) {
				oid.mohm = 6000;
				oid.mohm_real = mohm;
				oid.o2s[PIN_WHITE0] = px;
				oid.o2s[PIN_WHITE1] = py;
				n_wire ++;
			}
			else if(mohm < mohm_open_threshold) {
				n_rstrange ++;
				oid_error(E_STRANGE_RESISTOR + mohm);
			}
		}
	}

	/*error reporting*/
	int n_error_events = 0;
	if(n_rstrange > 0) {
		n_error_events += n_rstrange;
	}

	if(n_short_other > 0) {
		n_error_events += n_short_other;
		oid_error(E_SHORT_OTHER + n_short_other);
	}

	if(n_short_gnd > 1) {
		n_error_events += n_short_gnd - 1;
		oid_error(E_SHORT_SHELL_MORE + n_short_gnd);
	}
	else {
		if((oid.gnd != '?') && (oid.gnd != n_short_gnd)) {
			n_error_events ++;
			if(n_short_gnd == 0) {
				oid_error(E_OPEN_SHELL_GRAY);
			}
			else {
				oid_error(E_SHORT_SHELL_GRAY);
			}
		}
	}

	if(n_wire > 1) {
		n_error_events += n_wire - 1;
		oid_error(E_WIRE_MORE + n_wire);
	}
	else {
		if(oid.lines < 3) {
			n_error_events ++;
			oid_error(E_STRANGE_RESISTOR);
		}
	}

	//oxgen sensor grounded confirm
	if(oid.gnd == '?') {
		if(n_short_gnd == 0) {
			/*!!!warnning: if gray-shell connection is break?*/
			oid.gnd = 0;
		}
		else {
			if(oid.lines <= 2) {
				oid.gnd = 1;
			}
			else {
				if(n_wire == 1) { /*shell is shorten to heating wire?*/
					if((oid.o2s[PIN_GRAY] != oid.o2s[PIN_WHITE0]) && (oid.o2s[PIN_GRAY] != oid.o2s[PIN_WHITE1])) {
						oid.gnd = 1;
					}
				}
			}
		}
	}

	/*!!! i'll try my best to tell you the appropriate tcode, but it maybe incorrect in case of o2sensor is not good:(!!!*/
	oid.tcode = 0;
	switch(oid.lines) {
		case 1:
			oid.tcode = 0x1901;
			break;
		case 2:
			oid.tcode = (oid.gnd == 0) ? 0x2902 : oid.tcode;
			oid.tcode = (oid.gnd == 1) ? 0x2901 : oid.tcode;
			if(oid.gnd == 0) oid.o2s[PIN_GRAY] = oid_pin_search(0);
			break;
		case 3:
			oid.tcode = (oid.mohm == 2100) ? 0x3901 : oid.tcode;
			oid.tcode = (oid.mohm == 3500) ? 0x3902 : oid.tcode;
			oid.tcode = (oid.mohm == 6000) ? 0x3903 : oid.tcode;
			break;
		case 4:
			if(oid.gnd == 1) {
				oid.tcode = (oid.mohm == 2100) ? 0x4901 : oid.tcode;
				oid.tcode = (oid.mohm == 3500) ? 0x4902 : oid.tcode;
				oid.tcode = (oid.mohm == 6000) ? 0x4903 : oid.tcode;
			}
			if(oid.gnd == 0) {
				oid.tcode = (oid.mohm == 2100) ? 0x4904 : oid.tcode;
				oid.tcode = (oid.mohm == 3500) ? 0x4905 : oid.tcode;
				oid.tcode = (oid.mohm == 6000) ? 0x4906 : oid.tcode;
				oid.o2s[PIN_GRAY] = oid_pin_search(0);
			}
		default:;
	}

	/*hot up is needed only when good oxgen sensor is connected and its gray line is ungrounded,
	so black line & gray line could be identified by monitor the negative or positive voltage*/
	int hot_up_is_not_needed = 1;
	if(oid.mode == 'd') {
		oid.o2s[PIN_GRAY] = PIN_GRAY;
		oid.o2s[PIN_BLACK] = PIN_BLACK;
		oid.o2s[PIN_WHITE0] = PIN_WHITE0;
		oid.o2s[PIN_WHITE1] = PIN_WHITE1;
		hot_up_is_not_needed = 0;
	}
	else if(n_error_events == 0) {
		oid.o2s[PIN_BLACK] = oid_pin_search(0);
		if((oid.lines == 2) || (oid.lines == 4)) {
			if(oid.gnd == 0) {
				oid.o2s[PIN_GRAY] = oid_pin_search(0);
				hot_up_is_not_needed = 0;
			}
		}
	}

	return hot_up_is_not_needed;
}

static int oid_hot_up(void)
{
	int pin, uv, mv, ecode;
	struct debounce_s bake; /*trig condition: mv > 5*/
	struct debounce_s good; /*trig condition: mv > 750*/

	debounce_init(&bake, 10, 0);
	debounce_init(&good, 10, 0);

	pin = ((oid.lines == 2) || (oid.lines == 4)) ? PIN_GRAY : PIN_SHELL;
	mcd_pick(oid.o2s[pin], oid.o2s[PIN_BLACK]);
	mcd_mode(DMM_V_AUTO);

	oid.mv = 0;
	oid.scnt = 0;
	time_t timer = time_get(0);
	time_t deadline = time_get(oid.seconds * 1000U);
	int flag_pin_confirmed = 0;
	while(time_left(deadline) > 0) {
		if(oid.start == 1) {
			//user cancel
			oid.start = 0;
			oid.mv = 0;
			break;
		}

		ecode = mcd_read(&uv);
		if(ecode) {
			oid_error(E_SYSTEM);
			return 1;
		}

		mv = uv / 1000;
		mv = (mv < 0) ? -mv : mv;
		oid.mv = mv;

		//printf("O2 = %d mV\n", uv / 1000);
		if(debounce(&bake, (mv > 10))) {
			deadline = time_get(oid.seconds * 1000);
			if(bake.on) {
				if(uv < 0) { //swap the pin sequence of gray&black
					flag_pin_confirmed = 1;
					char pin = oid.o2s[PIN_GRAY];
					oid.o2s[PIN_GRAY] = oid.o2s[PIN_BLACK];
					oid.o2s[PIN_BLACK] = pin;
				}
			}
		}

		if(bake.on) { //count down
			oid.scnt = (time_left(deadline)/1000);
			if(debounce(&good, (mv > oid_mv_threshold))) {
				if(good.on) {
					break;
				}
			}
		}
		else { //count up
			oid.scnt = - (time_left(timer)/1000);
		}
	}

	if(flag_pin_confirmed == 0) { //reset to unknown
		oid.o2s[PIN_GRAY] = 0;
		oid.o2s[PIN_BLACK] = 0;
	}

	return (oid.mode == 'd') ? 0 : 1;
}

/*other failture will be reported in xxx strange resistor*/
static int oid_diagnosis(void)
{
	#define oid_check(fail, ecode) do {if(fail) oid_error(ecode);sys_update();} while(0)
	if(oid.gnd) oid_check(oid.mohms[PIN_SHELL][PIN_GRAY] > mohm_open_threshold, E_OPEN_SHELL_GRAY);
	else oid_check(oid.mohms[PIN_SHELL][PIN_GRAY] < _kohm(10), E_SHORT_SHELL_GRAY);
	oid_check(oid.mohms[PIN_SHELL][PIN_BLACK] < mohm_short_threshold, E_SHORT_SHELL_BLACK);
	oid_check(oid.mohms[PIN_SHELL][PIN_WHITE0] < mohm_short_threshold, E_SHORT_SHELL_WHITE);
	oid_check(oid.mohms[PIN_SHELL][PIN_WHITE1] < mohm_short_threshold, E_SHORT_SHELL_WHITE);

	oid_check(oid.mohms[PIN_GRAY][PIN_BLACK] < mohm_short_threshold, E_SHORT_GRAY_BLACK);
	oid_check(oid.mohms[PIN_GRAY][PIN_WHITE0] < mohm_short_threshold, E_SHORT_GRAY_WHITE);
	oid_check(oid.mohms[PIN_GRAY][PIN_WHITE1] < mohm_short_threshold, E_SHORT_GRAY_WHITE);

	oid_check(oid.mohms[PIN_BLACK][PIN_WHITE0] < mohm_short_threshold, E_SHORT_BLACK_WHITE);
	oid_check(oid.mohms[PIN_BLACK][PIN_WHITE1] < mohm_short_threshold, E_SHORT_BLACK_WHITE);

	oid_check(oid.mohms[PIN_WHITE0][PIN_WHITE1] < mohm_short_threshold, E_SHORT_WHITE_WHITE);
	oid_check(oid.mohms[PIN_WHITE0][PIN_WHITE1] > mohm_open_threshold, E_OPEN_WHITE_WHITE);

	oid_check(oid.mv < oid_mv_threshold, E_LOSE_HIGH_VOLTAGE);
	return 0;
}

/*generate kcode, when in identification mode, kcode = line sequence, like gbww or gwwb ..*/
static int oid_stop(void)
{
	int kcode = 0;
	if(oid.mode == 'i') { //kcode = correct line sequence order
		for(int i = 1; i < NR_OF_PINS; i ++) {
			kcode <<= 4;
			kcode += oid_pin_search(i);
		}
	}
	else { //kcode = lines + gnd + mohm(35 = 3500mohm)
		kcode += oid.lines;
		kcode <<= 4;
		kcode += oid.gnd;
		kcode <<= 4;
		kcode += oid.mohm_real / 1000;
		kcode <<= 4;
		kcode += (oid.mohm_real % 1000) / 100;
	}
	oid.kcode = kcode;
	sys_update();
	oid.start = 0;
	oid.lock = 0;
	return 0;
}

/* the distinguish between identification mode and diagnosis mode is:
1, the line sequence must be strictly follow Gray-Black-White-White when in diagnosis mode.
2, Hot up is needed by identification mode only when gray line is not grounded.
3, More detail info will be given by diagnosis mode
*/
static const struct tstep_s oid_steps[] = {
	{.test = oid_start, .pass = NULL, .fail = NULL},
	{.test = oid_self_check, .pass = NULL, .fail = NULL},
	{.test = oid_measure_resistors, .pass = NULL, .fail = NULL},
	{.test = oid_identify, .pass = NULL, .fail = NULL},
	{.test = oid_hot_up, .pass = NULL, .fail = NULL},
	{.test = oid_diagnosis, .pass = NULL, .fail = NULL},
	{.test = oid_stop, .pass = NULL, .fail = NULL},
};

void main(void)
{
	sys_init();
	oid_init();
	oid_gui_init();
	led_on(LED_GREEN);
	led_flash(LED_RED);
	tstep_execute(oid_steps, sizeof(oid_steps)/sizeof(struct tstep_s));
}

void __sys_update(void)
{
	oid_gui_update();
}
