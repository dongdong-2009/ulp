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

static mempool_t *gui_wpool;
static const unsigned gui_colormap_default[] = {BLUE, WHITE};

void gui_widget_init(void)
{
	gui_wpool = mempool_create(sizeof(gwidget), CONFIG_GUI_WPSZ);
}

void gui_widget_react(gwidget *widget, gevent *event)
{
	int result = -1;
	if(widget->usr_event_func != NULL)
		result = widget->usr_event_func(widget, event);
	if(result)
		widget->sys_event_func(widget, event);
}

gwidget* gui_widget_new(void)
{
	gwidget *widget = mempool_alloc(gui_wpool);
	assert(widget != NULL);
	widget->flag_show = 1;
	widget->flag_visible = 0;
	widget->flag_window = 0;
	widget->border_width = 4;

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
}

void gui_widget_show(gwidget *widget)
{
	widget->flag_show = 1;
}

void gui_widget_draw(gwidget *widget)
{
	gui_lcd_pen(widget->colormap[GUI_FGCOLOR], \
		widget->colormap[GUI_BGCOLOR], widget->border_width);
	gui_lcd_rectangle(widget->x, widget->y, \
		widget->w - widget->border_width, widget->h - widget->border_width);
}
