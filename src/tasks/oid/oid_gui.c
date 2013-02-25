/*
 * 	miaofng@2012 initial version
 */

#include "gui/gui.h"
#include <stdio.h>
#include "oid.h"
#include "string.h"
#include "ulp/sys.h"

struct oid_gui_s gui;
static char str[16]; /*str buffer for snprintf*/

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
static unsigned cmap_kcode[] = GUI_COLORMAP_NEW(RGB(0xe0, 0xe0, 0xe0), WHITE);

static int main_identify_on_event(gwidget *widget, gevent *event)
{
	static char str[5];
	if(event->type == GUI_TOUCH_BEGIN) {
		//printf("x = %d, y = %d\n", event->ts.x, event->ts.y);
		if(gui_widget_touched(widget, event)) {
			if(widget == button_lines) {
				if(oid.lock == 0) {
					oid.lines = (oid.lines < 4) ? (oid.lines + 1) : 1;
					sprintf(str, "%d", oid.lines);
					gui_widget_set_text(widget, str);
				}
			}
			else if(widget == button_ground) {
				if(oid.lock == 0) {
					char *p;
					switch(gui.gnd) {
					case '?':
						gui.gnd = 0;
						p = "N";
						break;
					case 0:
						gui.gnd = 1;
						p = "Y";
						break;
					default:
						gui.gnd = '?';
						p = "?";
						break;
					}
					gui_widget_set_text(widget, p);
				}
			}
			else if(widget == button_mode_dg) {
				if(oid.lock == 0) {
					oid.mode = 'd';
					gui_widget_set_text(button_mode_id, " ");
					gui_widget_set_text(button_mode_dg, "<");
				}
			}
			else if(widget == button_mode_id) {
				if(oid.lock == 0) {
					oid.mode = 'i';
					gui_widget_set_text(button_mode_id, "<");
					gui_widget_set_text(button_mode_dg, " ");
				}
			}
			else if(widget == button_start) {
				oid.start = 1;
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
	button_keycode = gui_button_new_with_label(" -- ");
	button_typecode = gui_button_new_with_label(" -- ");
	button_ecode0 = gui_button_new_with_label(" -- ");
	button_ecode1 = gui_button_new_with_label(" -- ");
	button_ecode2 = gui_button_new_with_label(" -- ");

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

void gui_progress_update(void)
{
	gwidget *widget = (oid.mode == 'i') ? button_mode_id : button_mode_dg;
	if(oid.lock != gui.lock) {
		gui.lock = oid.lock;
		if(gui.lock == 0) { //resume '<'
			char *p = "<";
			gui_widget_set_text(widget, p);
			return;
		}
	}

	if(oid.lock == 0) { //controlled by event loop
		return;
	}

	if(oid.scnt != gui.scnt) { //display digit
		gui.scnt = oid.scnt;
		sprintf(str, "%d", gui.scnt);
		gui_widget_set_text(widget, str);
		return;
	}

	if(oid.scnt == 0) {
		if(time_left(gui.timer) < 0) {
			gui.timer = time_get(1000);
			gui.pbar ++;
			gui.pbar = (gui.pbar < 3) ? gui.pbar : 0;
			
			char *p = ">";
			p = (gui.pbar == 0) ? ">  " : p;
			p = (gui.pbar == 1) ? ">> " : p;
			p = (gui.pbar == 2) ? ">>>" : p;
			gui_widget_set_text(widget, p);
		}
	}
}

static void gui_error_update(int show)
{
	gwidget *widget[] = {
		button_ecode0,
		button_ecode1,
		button_ecode2,
	};

	for(int i = 0; i < 3; i ++) {
		if(oid.ecode[i] != gui.ecode[i]) {
			int ecode = oid.ecode[i];
			gui.ecode[i] = ecode;
			switch(ecode) {
			case E_OK:
				snprintf(str, 7, "--");
				break;

			default:
				if(ecode & 0xff000000) {
					ecode -= 0x01000000;
					snprintf(str, 7, "?%05d", ecode);
				}
				else {
					snprintf(str, 7, "%06x", ecode);
				}
			}

			if(show == 0) strcpy(str, "      ");
			gui_widget_set_text(widget[i], str);
		}
	}
}

void gui_error_flash(void)
{
	for(int i = 0; i < 3; i ++) {
		gui_error_update(0);
		sys_mdelay(200);
		gui_error_update(1);
		sys_mdelay(100);
	}
}

void oid_gui_init(void)
{
	memset(&gui, 0xff, sizeof(gui));
	gui.gnd = '?';
	gui_init(NULL);
	main_window_init();
	gui_update(); //to disp the main window asap
}

void oid_gui_update(void)
{
	gui_update();
	gui_error_update(1);
	gui_progress_update();

	if(oid.kcode != gui.kcode) {
		gui.kcode = oid.kcode;
		sprintf(str, "%04x", gui.kcode);
		char *p = (gui.kcode == 0) ? " -- " : str;
		gui_widget_set_text(button_keycode, p);
	}
	
	if(oid.tcode != gui.tcode) {
		gui.tcode = oid.tcode;
		sprintf(str, "%04x", gui.tcode);
		char *p = (gui.tcode == 0) ? " -- " : str;
		gui_widget_set_text(button_typecode, p);
	}
	
	if(oid.scnt != 0) {
		if(oid.mv != gui.mv) {
			gui.mv = oid.mv;
			sprintf(str, "%04d", gui.mv);
			gui_widget_set_text(button_keycode, str);
		}
	}
}
