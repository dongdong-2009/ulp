/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui.h"

int gui_init(const char *cmdline)
{
	gui_lcd_init();
	gui_event_init();
	gui_widget_init();
	gui_window_init();
	return 0;
}

int gui_update(void)
{
	gui_event_update();
	gui_window_update();
	return 0;
}
