/*
 * 	miaofng@2012 initial version
 */
#ifndef __GUI_WIDGET_H_
#define __GUI_WIDGET_H_

#include "linux/list.h"

#define GUI_COLORMAP_NEW(fgcolor, bgcolor) {fgcolor, bgcolor}

typedef struct gui_window_s gwindow;
typedef struct gui_event_s gevent;
typedef struct gui_widget_s gwidget;
/*note: return 0 to avoid call sys_event_func again*/
typedef int(*gcallback)(gwidget *widget, gevent *event);

/*color map index*/
enum {
		GUI_FGCOLOR,
		GUI_BGCOLOR,
};

struct gui_widget_s {
	/*status bits*/
	unsigned flag_show :1;
	unsigned flag_visible :1;
	unsigned flag_window :1;
	unsigned border_width :3;

	/*allocated size*/
	short x;
	short y;
	short w;
	short h;

	/*size request*/
	short req_w;
	short req_h;

	const unsigned *image;
	const unsigned *colormap;

	/*event response callback*/
	gcallback sys_event_func;
	gcallback usr_event_func;
	//const void* sys_event_priv;
	const void* usr_event_priv;

	struct gui_widget_s *child;
	struct list_head list;

	/*specified propert*/
	union {
		void *priv;
		gwindow *window;
		const char *string;
	};
};

void gui_widget_init(void);
void gui_widget_react(gwidget *widget, gevent *event);
gwidget* gui_widget_new(void);
void gui_widget_del(gwidget *widget);
void gui_widget_set_size_request(gwidget *widget, int width, int height);
void gui_widget_show(gwidget *widget);
void gui_widget_set_border_width(gwidget *widget, int width);
void gui_widget_set_image(gwidget *widget, const void *image);
void gui_widget_add(gwidget *widget, gwidget *child);
void gui_widget_show_all(gwidget *widget);
void gui_widget_set_text(gwidget *widget, const char *text);
void gui_widget_set_colormap(gwidget *widget, const unsigned *colormap);
void gui_widget_draw(gwidget *widget);
int gui_widget_touched(gwidget *widget, gevent *event);

/*widget - label*/
gwidget* gui_label_new(const char *str);

/*widget - button*/
gwidget* gui_button_new(void);
gwidget* gui_button_new_with_label(const char *label);

/*widget - fixed*/
gwidget* gui_fixed_new(void);
void gui_fixed_put(gwidget *fixed, gwidget *widget, int x, int y);

#endif /*__GUI_WIDGET_H_*/
