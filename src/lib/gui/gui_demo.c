/*
 * 	miaofng@2012 initial version
 */

#include "ulp/sys.h"
#include "gui/gui.h"

static gwidget *main_window;
static gwidget *fixed;
static gwidget *button_identify;
static gwidget *button_diagnose;

void main_window_init(void)
{
	main_window = gui_window_new(GUI_WINDOW_TOPLEVEL);
	gui_widget_set_image(main_window, image_main_window);
	fixed = gui_fixed_new();
	gui_widget_add(main_window, fixed);

	button_identify = gui_button_new();
	button_diagnose = gui_button_new();
	gui_widget_set_size_request(button_identify, 100, 50);
	gui_widget_set_size_request(button_diagnose, 100, 50);
	gui_fixed_put(fixed, button_identify, 100, 100);
	gui_fixed_put(fixed, button_diagnose, 100, 100);
	gui_widget_show_all(main_window);
}

void main(void)
{
	sys_init();
	gui_init(NULL);
	main_window_init();
	while(1) {
		sys_update();
		gui_update();
	}
}
