/*
 * 	miaofng@2012 initial version
 */

#include "ulp/sys.h"
#include "gui/gui.h"
#include "instr/instr.h"
#include "instr/matrix.h"
#include "instr/dmm.h"
#include <string.h>

static gwidget *main_window;
static gwidget *fixed;
static gwidget *button_lines;
static gwidget *button_ground;
static gwidget *button_mode_id;
static gwidget *button_mode_dg;
static gwidget *button_start;
static gwidget *button_keycode;
static gwidget *button_typecode;
static gwidget *button_ecode0;
static gwidget *button_ecode1;
static gwidget *button_ecode2;

static unsigned cmap_ecode[] = GUI_COLORMAP_NEW(RED, WHITE);
static unsigned cmap_tcode[] = GUI_COLORMAP_NEW(GREEN, WHITE);
static unsigned cmap_kcode[] = GUI_COLORMAP_NEW(RGB(0xf0, 0xf0, 0xf0), WHITE);

static struct oid_config_s {
	unsigned start : 1;
	unsigned pause : 1;
	unsigned lines : 3;
	unsigned seconds : 9;
	unsigned mode : 8; //'i', 'd'
	const char *ground;
} oid_config;

enum {
	IDLE,
	BUSY,
	CAL,
	COLD,
	WAIT,
	HOT,
	FINISH,
};

static int oid_stm;

static int main_identify_on_event(gwidget *widget, gevent *event)
{
	static char str[5];
	if(event->type == GUI_TOUCH_BEGIN) {
		printf("x = %d, y = %d\n", event->ts.x, event->ts.y);
		if(gui_widget_touched(widget, event)) {
			if(widget == button_lines) {
				if(oid_stm == IDLE) {
					oid_config.lines = (oid_config.lines < 4) ? (oid_config.lines + 1) : 1;
					sprintf(str, "%d", oid_config.lines);
					gui_widget_set_text(widget, str);
				}
			}
			else if(widget == button_ground) {
				if(oid_stm == IDLE) {
					switch(oid_config.ground[0]) {
					case '?':
						oid_config.ground = "N";
						break;
					case 'N':
						oid_config.ground = "Y";
						break;
					default:
						oid_config.ground = "?";
						break;
					}
					gui_widget_set_text(widget, oid_config.ground);
				}
			}
			else if(widget == button_mode_dg) {
				if(oid_stm == IDLE) {
					oid_config.mode = 'd';
					gui_widget_set_text(button_mode_id, " ");
					gui_widget_set_text(button_mode_dg, "<");
				}
				else {
					if(oid_config.mode == 'd') {
						oid_config.pause = !oid_config.pause;
					}
				}
			}
			else if(widget == button_mode_id) {
				if(oid_stm == IDLE) {
					oid_config.mode = 'i';
					gui_widget_set_text(button_mode_id, "<");
					gui_widget_set_text(button_mode_dg, " ");
				}
				else {
					if(oid_config.mode == 'i') {
						oid_config.pause = !oid_config.pause;
					}
				}
			}
			else if(widget == button_start) {
				oid_config.start = 1;
			}
			else {
			/*
				i = (i < 19) ? (i + 1) : 0;
				sprintf(str, "%d", i);
				gui_widget_set_text(widget, str);
			*/
			}
		}
	}
	return -1;
}

void main_window_init(void)
{
	extern const unsigned char image_main_window[];
	main_window = gui_window_new(GUI_WINDOW_TOPLEVEL);
	gui_widget_set_image(main_window, image_main_window);
	fixed = gui_fixed_new();
	gui_widget_add(main_window, fixed);

	button_lines = gui_button_new_with_label("4");
	button_ground = gui_button_new_with_label("?");
	button_mode_id = gui_button_new_with_label("<");
	button_mode_dg = gui_button_new_with_label(" ");
	button_start = gui_button_new();
	button_keycode = gui_button_new_with_label("--");
	button_typecode = gui_button_new_with_label("--");
	button_ecode0 = gui_button_new_with_label("--");
	button_ecode1 = gui_button_new_with_label("--");
	button_ecode2 = gui_button_new_with_label("--");

	gui_widget_set_colormap(button_keycode, cmap_kcode);
	gui_widget_set_colormap(button_typecode, cmap_tcode);
	gui_widget_set_colormap(button_ecode0, cmap_ecode);
	gui_widget_set_colormap(button_ecode1, cmap_ecode);
	gui_widget_set_colormap(button_ecode2, cmap_ecode);

	gui_widget_set_size_request(button_lines, 60, 40-4);
	gui_widget_set_size_request(button_ground, 60, 40-4);
	gui_widget_set_size_request(button_mode_id, 60, 40-4);
	gui_widget_set_size_request(button_mode_dg, 60, 40-4);
	gui_widget_set_size_request(button_start, 120, 40-4);
	gui_widget_set_size_request(button_keycode, 116, 40-4);
	gui_widget_set_size_request(button_typecode, 116, 40-4);
	gui_widget_set_size_request(button_ecode0, 116, 40-4);
	gui_widget_set_size_request(button_ecode1, 116, 40-4);
	gui_widget_set_size_request(button_ecode2, 116, 40-4);
	gui_fixed_put(fixed, button_lines, 260, 40*1 + 2);
	gui_fixed_put(fixed, button_ground, 260, 40*2 + 2);
	gui_fixed_put(fixed, button_mode_dg, 260, 40*3 + 2);
	gui_fixed_put(fixed, button_mode_id, 260, 40*4 + 2);
	gui_fixed_put(fixed, button_start, 200, 40*5 + 2);
	gui_fixed_put(fixed, button_keycode, 80, 40*1 + 2);
	gui_fixed_put(fixed, button_typecode, 80, 40*2 + 2);
	gui_fixed_put(fixed, button_ecode0, 80, 40*3 + 2);
	gui_fixed_put(fixed, button_ecode1, 80, 40*4 + 2);
	gui_fixed_put(fixed, button_ecode2, 80, 40*5 + 2);
	gui_event_connect(button_lines, main_identify_on_event);
	gui_event_connect(button_ground, main_identify_on_event);
	gui_event_connect(button_mode_id, main_identify_on_event);
	gui_event_connect(button_mode_dg, main_identify_on_event);
	gui_event_connect(button_start, main_identify_on_event);
	gui_event_connect(button_keycode, main_identify_on_event);
	gui_event_connect(button_typecode, main_identify_on_event);
	gui_event_connect(button_ecode0, main_identify_on_event);
	gui_event_connect(button_ecode1, main_identify_on_event);
	gui_event_connect(button_ecode2, main_identify_on_event);
	gui_window_show(main_window);
}

static int oid_ecode[3];

enum oid_error_type {
	E_OK = 0,
	E_UNDEF = 0x1000000,
	E_SYS_ERR,
	E_DMM_INIT,
	E_MATRIX_INIT,
	E_RES_CAL,
	E_VOL_CAL,
};

static void oid_show_status(int tcode, int kcode)
{
	char buf[7];
	char *str;
	snprintf(buf, 7, "%04x", (kcode >> 8) &0xffff);
	str = (kcode == 0) ? "--" : buf;
	gui_widget_set_text(button_keycode, str);
	snprintf(buf, 7, "%04d", tcode%10000);
	str = (tcode == 0) ? "--" : buf;
	gui_widget_set_text(button_typecode, str);
}

static void oid_error_show(int show)
{
	char str[7];
	int ecode;
	gwidget *widget[] = {
		button_ecode0,
		button_ecode1,
		button_ecode2,
	};

	for(int i = 0; i < 3; i ++) {
		ecode = oid_ecode[i];
		switch(ecode) {
		case E_OK:
			snprintf(str, 7, "--");
			break;

		case E_UNDEF:
			snprintf(str, 7, "UNDF%02d", ecode%100);
			break;

		case E_DMM_INIT:
			snprintf(str, 7, "DMMI%02d", ecode%100);
			break;

		case E_MATRIX_INIT:
			snprintf(str, 7, "MATI%02d", ecode%100);
			break;

		case E_RES_CAL:
			snprintf(str, 7, "RCAL%02d", ecode%100);
			break;

		case E_VOL_CAL:
			snprintf(str, 7, "VCAL%02d", ecode%100);
			break;

		default:
			snprintf(str, 7, "%06d", ecode%1000000);
		}

		if(show == 0) strcpy(str, "      ");
		gui_widget_set_text(widget[i], str);
	}
}

static void oid_error_flash(void)
{
	for(int i = 0; i < 3; i ++) {
		oid_error_show(0);
		sys_mdelay(200);
		oid_error_show(1);
		sys_mdelay(100);
	}
}

static inline void oid_error_init(void)
{
	oid_ecode[0] = E_OK;
	oid_ecode[1] = E_OK;
	oid_ecode[2] = E_OK;
	oid_error_show(1);
}

static void oid_error(int ecode)
{
	if(ecode != E_OK) {
		for(int i = 0; i < 3; i ++) {
			if(oid_ecode[i] == E_OK) {
				oid_ecode[i] = ecode;
				break;
			}
		}
		oid_error_show(1);
	}
}

enum {
	PROGRESS_TEST_START = -100,
	PROGRESS_TEST_STOP,
	PROGRESS_WAIT,
};

static void oid_show_progress(int value)
{
	char buf[4];
	char *str;
	static char wait = 0;
	gwidget *widget = (oid_config.mode == 'i') ? button_mode_id : button_mode_dg;
	switch(value) {
	case PROGRESS_TEST_START:
		wait = 0;
	case PROGRESS_WAIT:
		switch(wait) {
		case 0:
			str = ">";
			wait = 1;
			break;

		case 1:
			str = ">>";
			wait = 2;
			break;

		default:
			str = ">>>";
			wait = 0;
		}
		break;

	case PROGRESS_TEST_STOP:
		str = "<";
		break;

	default:
		sprintf(buf, "%d", value);
		str = buf;
		break;
	}

	gui_widget_set_text(widget, str);
}

int oid_wait(void)
{
	oid_config.start = 0;
	oid_config.pause = 0;
	for(int i = oid_config.seconds; i > 0; i --) {
		oid_show_progress(i);

		//wait 1s
		time_t deadline = time_get(1000);
		while((time_left(deadline) > 0) || (oid_config.pause == 1)) {
			sys_update();
			instr_update();
			gui_update();
			if(oid_config.start == 1) { //start key is pressed again, give up the test ...
				return 1;
			}
		}
	}
	return 0;
}

void oid_init(void)
{
	oid_config.lines = 4;
	oid_config.ground = "?";
	oid_config.mode = 'i';
	oid_config.seconds = 30;
	oid_config.start = 0;
	oid_stm = IDLE;
}

#include "oid.h"

void oid_update(void)
{
	if(oid_config.start == 0)
		return;

	oid_stm = BUSY;
	oid_error_init();
	oid_show_progress(PROGRESS_TEST_START);
	oid_show_status(0, 0);

	int giveup = 0;
	while((!giveup) && (oid_stm < FINISH)) {
		oid_stm ++;
		switch(oid_stm) {
		case CAL:
			giveup = oid_instr_cal();
			break;

		case COLD:
			giveup = oid_cold_test(&oid_config);
			break;

		case WAIT:
			giveup = oid_wait();
			break;

		case HOT:
			giveup = oid_hot_test(&oid_config);
			break;

		default:
			giveup = 1;
			break;
		}
	}

	if(giveup) {
		oid_error_flash();
	}

	oid_show_progress(PROGRESS_TEST_STOP);
	oid_config.start = 0;
	oid_stm = IDLE;
}

void main(void)
{
	sys_init();
	instr_init();
	oid_init();
	gui_init(NULL);
	main_window_init();
	gui_update(); //to disp the main window asap
	while(1) {
		sys_update();
		instr_update();
		oid_update();
		gui_update();
	}
}
