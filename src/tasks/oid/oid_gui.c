/*
 * 	miaofng@2012 initial version
 *	miaofng@2013/7/25 enhanced gui to totaly seprate gui thread and oid main thread
 */

#include "gui/gui.h"
#include <stdio.h>
#include "oid.h"
#include "string.h"
#include "ulp/sys.h"

static gwidget *main_window;
static gwidget *fixed;
static gwidget *button_lines; //update when pressed
static gwidget *button_ground; //update when pressed
static gwidget *button_mode_id; //update when id/dg pressed
static gwidget *button_mode_dg; //update when id/dg pressed
static gwidget *button_start;
static gwidget *button_keycode; //update manually when necessary
static gwidget *button_typecode; //update manually when necessary
static gwidget *button_ecode0; //update manually when necessary
static gwidget *button_ecode1; //update manually when necessary
static gwidget *button_ecode2; //update manually when necessary
static gwidget *boot_timer;

static unsigned cmap_ecode[] = GUI_COLORMAP_NEW(RED, WHITE);
static unsigned cmap_tcode[] = GUI_COLORMAP_NEW(GREEN, WHITE);
static unsigned cmap_kcode[] = GUI_COLORMAP_NEW(RGB(0xe0, 0xe0, 0xe0), WHITE);

static char str[16]; /*str buffer for snprintf*/
static struct o2s_config_s gui_config;
static struct oid_result_s gui_result;
static time_t gui_timer;
static char gui_counter;
static char gui_busy;
static char gui_flash;

static int main_identify_on_event(gwidget *widget, gevent *event)
{
	if(event->type != GUI_TOUCH_BEGIN)
		return -1;

	//printf("x = %d, y = %d\n", event->ts.x, event->ts.y);
	if(!gui_widget_touched(widget, event))
		return -1;

	if(widget == button_start) {
		if(gui_flash == 0) {
			oid_set_config(&gui_config);
			oid_start();
			return -1;
		}
	}

	if(oid_is_busy())
		return -1;

	if(widget == button_lines) {
		gui_config.lines = (gui_config.lines < 4) ? (gui_config.lines + 1) : 1;
		sprintf(str, "%d", gui_config.lines);
		gui_widget_set_text(widget, str);
		oid_set_config(&gui_config);
		if((gui_config.lines == 1) || (gui_config.lines == 3)) {
			gui_widget_set_text(button_ground, "-");
		}
		else {
			static const char *grounded = "?YN?";
			int i;
			for(i = 0; grounded[i] != gui_config.grounded; i ++);
			memset(str, 0x00, sizeof(str));
			str[0] = gui_config.grounded;
			gui_widget_set_text(button_ground, str);
		}
	}

	else if(widget == button_ground) {
		if((gui_config.lines == 2) || (gui_config.lines == 4)) {
			static const char *grounded = "?YN?";
			int i;
			for(i = 0; grounded[i] != gui_config.grounded; i ++);
			gui_config.grounded = grounded[++ i];
			memset(str, 0x00, sizeof(str));
			str[0] = gui_config.grounded;
			gui_widget_set_text(widget, str);
			oid_set_config(&gui_config);
		}
	}

	else if((widget == button_mode_dg) || (widget == button_mode_id)) {
		char mode = (widget == button_mode_dg)? 'D' : 'I';
		if(mode != gui_config.mode) {
			gui_widget_set_text(button_mode_id, (mode == 'I') ? "<" : " ");
			gui_widget_set_text(button_mode_dg, (mode == 'D') ? "<" : " ");
			gui_config.mode = mode;
			oid_set_config(&gui_config);
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
	boot_timer = gui_button_new_with_label("5");

	gui_widget_set_colormap(button_keycode, cmap_kcode);
	gui_widget_set_colormap(button_typecode, cmap_tcode);
	gui_widget_set_colormap(button_ecode0, cmap_ecode);
	gui_widget_set_colormap(button_ecode1, cmap_ecode);
	gui_widget_set_colormap(button_ecode2, cmap_ecode);

	gui_widget_set_size_request(button_lines, 60, 40-4);
	gui_widget_set_size_request(button_ground, 60, 40-4);
	gui_widget_set_size_request(button_mode_id, 60, 40-4);
	gui_widget_set_size_request(button_mode_dg, 60, 40-4);
	gui_widget_set_size_request(button_start, 120, 40-4);//
	gui_widget_set_size_request(boot_timer, 20, 40-4);
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
	gui_fixed_put(fixed, boot_timer, 295, 40*5 + 2);
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


//110 + 14 * i, 46
static void oid_draw_box(int x, int y, int w, int h, int color)
{
	gui_lcd_pen(BLACK, WHITE, 1);
	gui_lcd_rectangle(x, y, w, h);
	gui_lcd_pen(color, WHITE, 1);
	gui_lcd_box(x + 1, y + 1, w - 2, h - 2);
}

#if 1
#include "shell/cmd.h"

static int cmd_box_func(int argc, char *argv[])
{
	if(argc >= 6) {
		int x = atoi(argv[1]);
		int y = atoi(argv[2]);
		int w = atoi(argv[3]);
		int h = atoi(argv[4]);
		int c = htoi(argv[5]);
		int r = (c >> 16) & 0xff;
		int g = (c >> 8) & 0xff;
		int b = (c >> 0) & 0xff;
		oid_draw_box(x, y, w, h, RGB(r, g, b));
	}
}

const cmd_t cmd_box = {"box", cmd_box_func, "draw box at specified position"};
DECLARE_SHELL_CMD(cmd_box)
#endif

void oid_gui_init(void)
{
	gui_config.lines = 4;
	gui_config.mode = 'I';
	gui_config.grounded = '?';
	memset(&gui_result, 0x00, sizeof(gui_result));
	gui_busy = 0;

	gui_init(NULL);
	main_window_init();
	gui_update(); //to disp the main window asap

	//to display boot timer
	for(int i = 26; i > 0; i --) {
		const char *p[] = {"-", "/", "|", "\\"};
		gui_widget_set_text(boot_timer, p[i&0x03]);
		mdelay(200);
	}
	gui_widget_set_text(boot_timer, " ");
}

void oid_gui_update(void)
{
	struct oid_result_s oid_result;
	oid_get_result(&oid_result);
	gwidget *widget = (gui_config.mode == 'I') ? button_mode_id : button_mode_dg;

	//start & stop event detection
	char restore = 0;
	char oid_busy = oid_is_busy();
	if(oid_busy != gui_busy) {
		gui_busy = oid_busy;
		gui_timer = time_get(0);
		restore = 1;

		if(oid_busy) { //idle -> busy
			gui_counter = 0;
			memset(&gui_result, 0x00, sizeof(gui_result));
		}
		else { //busy->idle
			gui_flash = 6;
		}
	}

	if(restore || (oid_result.kcode != gui_result.kcode)) {
		if(gui_config.mode == 'D') {
			sprintf(str, "%04x", oid_result.kcode);
			char *p = (oid_result.kcode == 0) ? " -- " : str;
			gui_widget_set_text(button_keycode, p);
		}
		else {
			static const int colors[NR_OF_PINS] = { RED, GRAY, BLACK, WHITE, WHITE};
			memset(str, 0x00, sizeof(str));
			for(int i = 0; i < 4; i ++) {
				int idx = (oid_result.kcode >> (3 - i) * 4) & 0x0f;
				str[i] = (idx == 0) ? '?' : ' ';
				if(i + 1 > gui_config.lines) {
					str[i] = '-';
				}
			}
			gui_widget_set_text(button_keycode, str);

			for(int i = 0; i < 4; i ++) {
				int idx = (oid_result.kcode >> (3 - i) * 4) & 0x0f;
				idx %= 5;
				if(idx != 0) {
					oid_draw_box(110 + 16 * i, 46, 10, 28, colors[idx]);
				}
			}
		}
	}

	if(restore || (oid_result.tcode != gui_result.tcode)) {
		sprintf(str, "%04x", oid_result.tcode);
		char *p = (oid_result.tcode == 0) ? " -- " : str;
		gui_widget_set_text(button_typecode, p);
	}

	//diplay o2s mv at kcode position
	if(oid_busy) {
		if(oid_is_hot()) {
			static int mv_disp = 0;
			int mv = oid_hot_get_mv();
			mv = (mv > 9999) ? 9999 : mv; /*to avoid display digits too long*/
			if(mv != mv_disp) {
				mv_disp = mv;
				sprintf(str, "%04d", mv);
				gui_widget_set_text(button_keycode, str);
			}
		}
	}

	//progress bar update
	if(restore) gui_widget_set_text(widget, "<");
	if(oid_busy) {
		if(time_left(gui_timer) <= 0) {
			gui_timer = time_get(1000);

			if(oid_is_hot()) {
				//display 90 89 88 ...
				int ms = oid_hot_get_ms();
				sprintf(str, "%d", ms / 1000);
				gui_widget_set_text(widget, str);
			}
			else { //display >>>
				static const char *pstr[] = {">  ", ">> ", ">>>"};
				gui_widget_set_text(widget, pstr[gui_counter]);
				gui_counter ++;
				gui_counter %= 3;
			}
		}
	}

	//ecode handling
	gwidget *button_ecode[] = {
		button_ecode0,
		button_ecode1,
		button_ecode2,
	};

	//ecode flash?
	int force = 0;
	if(gui_flash > 0) {
		if(time_left(gui_timer) <= 0) {
			gui_timer = time_get(300);
			gui_flash --;

			//force ecode display update
			force = 1;
		}
	}

	//ecode display
	for(int i = 0; i < 3; i ++) {
		if(restore || force || (oid_result.ecode[i] != gui_result.ecode[i])) {
			sprintf(str, "%06x", oid_result.ecode[i]);
			char *p = (oid_result.ecode[i] == 0) ? " -- " : str;
			if(oid_result.ecode[i] != 0x00) {
				p = (gui_flash & 0x01) ? "      " : p;
			}
			gui_widget_set_text(button_ecode[i], p);
		}
	}

	gui_update();
	memcpy(&gui_result, &oid_result, sizeof(gui_result));
}
