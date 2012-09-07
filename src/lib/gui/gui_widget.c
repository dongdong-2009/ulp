/*
 * 	miaofng@2012 initial version
 *
 */

#include <ulp/sys.h>
#include "gui/gui_config.h"
#include "gui/gui_widget.h"
#include "gui/gui_lcd.h"
#include "common/mempool.h"
#include <stdlib.h>
#include "common/color.h"
#include "gui/gui_event.h"

static mempool_t *gui_wpool;
static const unsigned gui_colormap_default[] = {BLACK, WHITE};

void gui_widget_init(void)
{
	gui_wpool = mempool_create(sizeof(gwidget), CONFIG_GUI_WPSZ);
}

void gui_widget_react(gwidget *widget, gevent *event)
{
	int result = -1;
	if(widget != NULL) {
		if(widget->usr_event_func != NULL)
			result = widget->usr_event_func(widget, event);
		if(result)
			widget->sys_event_func(widget, event);
	}
}

gwidget* gui_widget_new(void)
{
	gwidget *widget = mempool_alloc(gui_wpool);
	assert(widget != NULL);
	widget->flag_show = 1;
	widget->flag_visible = 0;
	widget->flag_window = 0;
	widget->border_width = 0;

	widget->x = 0;
	widget->y = 0;
	widget->w = 0;
	widget->h = 0;

	widget->req_w = 0;
	widget->req_h = 0;

	widget->image = NULL;
	widget->colormap = gui_colormap_default;

	widget->sys_event_func = NULL;
	widget->usr_event_func = NULL;
	widget->usr_event_priv = NULL;

	INIT_LIST_HEAD(&widget->list);
	widget->child = NULL;
	widget->priv = NULL;
	return widget;
}

void gui_widget_del(gwidget *widget)
{
	mempool_free(gui_wpool, widget);
}

void gui_widget_set_size_request(gwidget *widget, int width, int height)
{
	widget->req_w = width;
	widget->req_h = height;
}

void gui_widget_set_image(gwidget *widget, const void *image)
{
	widget->image = image;
}

void gui_widget_set_border_width(gwidget *widget, int width)
{
	widget->border_width = width;
}

void gui_widget_add(gwidget *widget, gwidget *child)
{
	widget->child = child;
	child->x = widget->x;
	child->y = widget->y;
	child->w = child->req_w;
	child->h = child->req_h;
}

void gui_widget_show(gwidget *widget)
{
	widget->flag_show = 1;
}

void gui_widget_set_text(gwidget *widget, const char *text)
{
	int x, y, w, h, bw, fw, fh, n;
	if(text == NULL)
		return;
	unsigned fgcolor = widget->colormap[GUI_FGCOLOR];
	unsigned bgcolor = widget->colormap[GUI_BGCOLOR];
	gui_lcd_pen(fgcolor, bgcolor, widget->border_width);
	gui_lcd_get_font(&fw, &fh);
	fw *= strlen(text);
	x = widget->x;
	y = widget->y;
	w = widget->w;
	h = widget->h;
	bw = widget->border_width;
	gui_lcd_clear(x + bw, y + bw, w - (bw << 1), h - (bw << 1));
	x += (w - fw) >> 1;
	y += (h - fh) >> 1;
	w -= (w - fw);
	h -= (h - fh);
	gui_lcd_puts(x, y, text);
	widget->string = text;
}

void gui_widget_set_colormap(gwidget *widget, const unsigned *colormap)
{
	widget->colormap = colormap;
}

void gui_widget_draw(gwidget *widget)
{
	int x, y, w, h;
	x = widget->x;
	y = widget->y;
	w = widget->w;
	h = widget->h;
	if(widget->image != NULL) {
		gui_lcd_imageblt(widget->image, x, y, w, h);
	}
	if(widget->border_width != 0) {
		unsigned fgcolor = widget->colormap[GUI_FGCOLOR];
		unsigned bgcolor = widget->colormap[GUI_BGCOLOR];
		gui_lcd_pen(fgcolor, bgcolor, widget->border_width);
		gui_lcd_rectangle(x, y, w - widget->border_width, h - widget->border_width);
	}
}

void gui_widget_clear(gwidget *widget)
{
	int x, y, w, h, bw;
	x = widget->x;
	y = widget->y;
	w = widget->w;
	h = widget->h;
	bw = widget->border_width;
	x += bw << 1;
	y += bw << 1;
	w -= bw << 2;
	h -= bw << 2;
	//gui_lcd_clear(x, y, w, h);
	gui_lcd_rectangle(x, y, w, h);
}

int gui_widget_touched(gwidget *widget, gevent *event)
{
	return (
		(event->ts.x > widget->x) &&
		(event->ts.y > widget->y) &&
		(event->ts.x < widget->x + widget->w) &&
		(event->ts.y < widget->y + widget->h)
	);
}
