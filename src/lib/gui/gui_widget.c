/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_widget.h"
#include "common/mempool.h"

static mempool_t *gui_wpool;

void gui_widget_init(void)
{
	gui_wpool = mempool_create(sizeof(gwidget), CONFIG_GUI_WPSZ);
}

gwidget* gui_widget_new(void)
{
	gwidget *widget = mempool_alloc(gui_wpool);
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

void gui_widget_show(gwidget *widget)
{
	widget->show = 1;
}

void gui_widget_set_image(gwidget *widget, const void *image)
{
}

void gui_widget_set_border_width(gwidget *widget, int width)
{
	widget->border_width = width;
}

void gui_widget_add(gwidget *widget, gwidget *child)
{
	widget->child = child;
}

void gui_widget_show_all(gwidget *widget)
{

}

