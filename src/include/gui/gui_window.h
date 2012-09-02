/*
 * 	miaofng@2012 initial version
 */
#ifndef __GUI_WINDOW_H_
#define __GUI_WINDOW_H_

#include "linux/list.h"
#include "gui/gui_widget.h"
#include "ulp_time.h"

/*special widget - window, realized in gui.c*/
typedef enum {
	GUI_WINDOW_TOPLEVEL,
	GUI_WINDOW_POPUP,
} gwindow_type;

typedef struct gui_window_s {
	gwidget widget;
	time_t active_time;
	struct list_head equeue; /*head of event queue*/
	struct list_head list;
} gwindow;

gwidget* gui_window_new(gwindow_type type);
void gui_window_del(gwindow *window);
void gui_window_init(void);
int gui_window_update(void);
int gui_window_show(gwidget *widget);
void gui_window_destroy(gwidget *widget);
gwindow *gui_window_active(void);

#endif /*__GUI_WINDOW_H_*/
