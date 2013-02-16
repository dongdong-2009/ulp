/*
 * 	miaofng@2012 initial version
 */

#include "gui/gui.h"
#include <stdio.h>
#include "oid_gui.h"
#include "string.h"

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

static void main_window_init(void)
{
	extern const unsigned char gui_demo_bmp_bin[];
	main_window = gui_window_new(GUI_WINDOW_TOPLEVEL);
	gui_widget_set_image(main_window, gui_demo_bmp_bin);
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

void oid_gui_init(void)
{
	gui_init(NULL);
	main_window_init();
	gui_update(); //to disp the main window asap
}

void oid_show_progress(int value)
{
	char buf[4];
	char *str;
	static char wait = 0;
	gwidget *widget = (oid_config.mode == 'i') ? button_mode_id : button_mode_dg;
	switch(value) {
	case PROGRESS_START:
		wait = 0;
	case PROGRESS_BUSY:
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

	case PROGRESS_STOP:
		str = "<";
		break;

	default:
		sprintf(buf, "%d", value);
		str = buf;
		break;
	}

	gui_widget_set_text(widget, str);
}

void oid_show_result(int tcode, int kcode)
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

/*error handling*/
static int oid_ecode[3];
static void oid_error_show(int enable)
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

		if(!enable) strcpy(str, "      ");
		gui_widget_set_text(widget[i], str);
	}
}

void oid_error_init(void)
{
	oid_ecode[0] = E_OK;
	oid_ecode[1] = E_OK;
	oid_ecode[2] = E_OK;
	oid_error_show(1);
}

void oid_error(int ecode)
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

void oid_error_flash(void)
{
	for(int i = 0; i < 3; i ++) {
		oid_error_show(0);
		oid_mdelay(200);
		oid_error_show(1);
		oid_mdelay(100);
	}
}
