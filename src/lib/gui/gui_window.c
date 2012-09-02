/*
 * 	miaofng@2012 initial version
 *	- gui window manager
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_widget.h"
#include "gui/gui_window.h"
#include "gui/gui_event.h"
#include "linux/list.h"
#include "ulp/sys.h"

static struct list_head gui_windows;
static gwindow *gui_window_cache;

static int gui_window_on_expose(gwidget *widget, gevent *event, void *data)
{
	gwindow *window = (gwindow *) widget;
	list_add_tail(&gui_windows, &window->list);
	window->active_time = time_get(0);

	gui_widget_react(widget->child, event);
	return 0;
}

static int gui_window_on_touch(gwidget *widget, gevent *event, void *data)
{
	return 0;
}

static int gui_window_on_destroy(gwidget *widget, gevent *event, void *data)
{
	gwindow *window = (gwindow *) widget;
	gui_widget_react(widget->child, event);
	list_del(&window->list);

	gui_window_cache = (gui_window_cache == window) ? NULL : gui_window_cache;
	gui_window_del(window);
	return 0;
}

static int gui_window_event_func(gwidget *widget, gevent *event, void *data)
{
	int ret = -1;
	switch(event->type) {
	case GUI_EXPOSE:
		ret = gui_window_on_expose(widget, event, data);
		break;
	case GUI_DESTROY:
		ret = gui_window_on_destroy(widget, event, data);
		break;
	case GUI_TOUCH_BEGIN:
	case GUI_TOUCH_UPDATE:
	case GUI_TOUCH_END:
		ret = gui_window_on_touch(widget, event, data);
		break;
	default:
		;
	}

	gui_event_del(event);
	return ret;
}

gwidget* gui_window_new(gwindow_type type)
{
	gwidget *widget;
	gwindow *window = sys_malloc(sizeof(gwindow));
	assert(window != NULL);

	widget = &window->widget;
	widget->sys_event_func = gui_window_event_func;
	widget->sys_event_priv = NULL;
	return widget;
}

void gui_window_del(gwindow *window)
{
	sys_free(window);
}


/*please do not directly use gui_window_cache, use this function instead!!!*/
gwindow *gui_window_active(void)
{
	struct gui_window_s *window;
	struct list_head *pos;
	time_t active_time = 0;

	if(gui_window_cache == NULL) {  /*To find an newest history active window*/
		list_for_each(pos, &gui_windows) {
			window = list_entry(pos, gui_window_s, list);
			if(time_diff(window->active_time, active_time) >= 0) {
				active_time = window->active_time;
				gui_window_cache = window;
			}
		}
	}
	return gui_window_cache;
}

void gui_window_init(void)
{
	INIT_LIST_HEAD(&gui_windows);
	gui_window_cache = NULL;
}

int gui_window_update(void)
{
	struct gui_window_s *window;
	struct list_head *pos;
	gevent *event;

	list_for_each(pos, &gui_windows) {
		window = list_entry(pos, gui_window_s, list);
		for(event = gui_event_recv(window); event != NULL; ) {
			gui_widget_react(&window->widget, event);
		}
	}
	return 0;
}

/*will be called by gui_widget_show_all()*/
int gui_window_show(gwidget *widget)
{
	gevent *event;
	gwindow *window = (gwindow *) widget;
	assert(widget->window == 1);

	event = gui_event_new(GUI_EXPOSE, widget);
	assert(event != NULL);
	gui_event_emit(window, event);
	return 0;
}

void gui_window_destroy(gwidget *widget)
{
	gevent *event;
	gwindow *window = (gwindow *) widget;
	assert(widget->window == 1);

	event = gui_event_new(GUI_DESTROY, widget);
	assert(event != NULL);
	gui_event_emit(window, event);
}
