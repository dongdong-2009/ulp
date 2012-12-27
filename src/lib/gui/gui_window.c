/*
 * 	miaofng@2012 initial version
 *	- gui window manager
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_widget.h"
#include "gui/gui_window.h"
#include "gui/gui_event.h"
#include "gui/gui_lcd.h"
#include "linux/list.h"
#include "ulp/sys.h"
#include "common/color.h"

static struct list_head gui_windows;
static gwindow *gui_window_cache;

static int gui_window_on_expose(gwidget *widget, gevent *event)
{
	gwindow *window = widget->window;
	window->active_time = time_get(0);

	gui_widget_draw(widget);
	gui_widget_react(widget->child, event);
	return 0;
}

static int gui_window_on_touch(gwidget *widget, gevent *event)
{
	int x, y, w, h;
/*
	if(event->type == GUI_TOUCH_BEGIN) {
		x = event->ts.x - 1;
		y = event->ts.y - 1;
		w = 2;
		h = 2;
		gui_lcd_pen(BLUE, WHITE, 1);
		gui_lcd_rectangle(x, y, w, h);
	}
*/
	gui_widget_react(widget->child, event);
	return 0;
}

static int gui_window_on_destroy(gwidget *widget, gevent *event)
{
	gwindow *window = widget->window;
	gui_widget_react(widget->child, event);
	list_del(&window->list);

	gui_window_cache = (gui_window_cache == window) ? NULL : gui_window_cache;
	gui_window_del(window);
	return 0;
}

static int gui_window_event_func(gwidget *widget, gevent *event)
{
	int ret = -1;
	switch(event->type) {
	case GUI_EXPOSE:
		ret = gui_window_on_expose(widget, event);
		break;
	case GUI_DESTROY:
		ret = gui_window_on_destroy(widget, event);
		break;
	case GUI_TOUCH_BEGIN:
	case GUI_TOUCH_UPDATE:
	case GUI_TOUCH_END:
		ret = gui_window_on_touch(widget, event);
		break;
	default:
		;
	}

	gui_event_del(event);
	return ret;
}

gwidget* gui_window_new(gwindow_type type)
{
	int w, h;
	gwidget *widget;
	gwindow *window = sys_malloc(sizeof(gwindow));
	assert(window != NULL);

	widget = gui_widget_new();
	assert(widget != NULL);
	widget->flag_window = 1;
	widget->sys_event_func = gui_window_event_func;
	gui_lcd_get_resolution(&w, &h);
	widget->x = 0;
	widget->y = 0;
	widget->w = w;
	widget->h = h;
	widget->window = window;

	window->widget = widget;
	window->active_time = 0;
	INIT_LIST_HEAD(&window->equeue);
	INIT_LIST_HEAD(&window->list);
	return widget;
}

void gui_window_del(gwindow *window)
{
	gui_widget_del(window->widget);
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

	//fetch event from public event queue
	event = gui_event_recv(NULL);
	while(event != NULL) {
		list_for_each(pos, &gui_windows) {
			window = list_entry(pos, gui_window_s, list);
			gui_widget_react(window->widget, event);
		}
		event = gui_event_recv(NULL);
	}

	//fetch event from private event queue
	list_for_each(pos, &gui_windows) {
		window = list_entry(pos, gui_window_s, list);
		event = gui_event_recv(window);
		while (event != NULL) {
			gui_widget_react(window->widget, event);
			event = gui_event_recv(window);
		}
	}
	return 0;
}

/*will be called by gui_widget_show_all()*/
int gui_window_show(gwidget *widget)
{
	gevent *event;
	gwindow *window = widget->window;
	assert(widget->flag_window == 1);

	list_add_tail(&gui_windows, &window->list);
	event = gui_event_new(GUI_EXPOSE, widget);
	assert(event != NULL);
	gui_event_emit(window, event);
	return 0;
}

void gui_window_destroy(gwidget *widget)
{
	gevent *event;
	gwindow *window = widget->window;
	assert(widget->flag_window == 1);

	event = gui_event_new(GUI_DESTROY, widget);
	assert(event != NULL);
	gui_event_emit(window, event);
}
