/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_widget.h"
#include "gui/gui_event.h"

static int gui_fixed_on_draw(gwidget *widget, gevent *event)
{
	struct gui_widget_s *child;
	struct list_head *pos;

	list_for_each(pos, &widget->list) {
		child = list_entry(pos, gui_widget_s, list);
		gui_widget_react(child, event);
	}
	return 0;
}

static int gui_fixed_on_touch(gwidget *widget, gevent *event)
{
	struct gui_widget_s *child;
	struct list_head *pos;

	list_for_each(pos, &widget->list) {
		child = list_entry(pos, gui_widget_s, list);
		gui_widget_react(child, event);
	}
	return 0;
}

static int gui_fixed_event_func(gwidget *widget, gevent *event)
{
	int ret = -1;
	switch(event->type) {
	case GUI_EXPOSE:
		ret = gui_fixed_on_draw(widget, event);
		break;
	case GUI_TOUCH_BEGIN:
	case GUI_TOUCH_UPDATE:
	case GUI_TOUCH_END:
		ret = gui_fixed_on_touch(widget, event);
		break;
	default:
		;
	}
	return ret;
}

gwidget* gui_fixed_new(void)
{
	gwidget *widget = gui_widget_new();
	widget->sys_event_func = gui_fixed_event_func;
	return widget;
}

void gui_fixed_put(gwidget *fixed, gwidget *widget, int x, int y)
{
	widget->x = fixed->x + x;
	widget->y = fixed->y + y;
	widget->w = widget->req_w;
	widget->h = widget->req_h;
	list_add_tail(&widget->list, &fixed->list);
}
