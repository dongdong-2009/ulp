/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_event.h"
#include "common/mempool.h"
#include "linux/list.h"

static mempool_t *gui_epool;

void gui_event_init(void)
{
	gui_epool = mempool_create(sizeof(gevent), CONFIG_GUI_EPSZ);
}

void gui_event_update(void)
{
}

gevent* gui_event_new(gevent_type type, gwidget *dst)
{
	gevent *event = mempool_alloc(gui_epool);
	event->type = type;
	event->dst = dst;
	INIT_LIST_HEAD(&event->list);
	return event;
}

void gui_event_del(gevent *event)
{
	mempool_free(gui_epool, event);
}

int gui_event_connect(gwidget *dst, gcallback func, const void *priv)
{
	dst->usr_event_func = func;
	dst->usr_event_priv = priv;
	return 0;
}

/*gwindow event queue*/
int gui_event_emit(gwindow *window, gevent *event)
{
	list_add_tail(&event->list, &window->equeue);
	return 0;
}

gevent* gui_event_recv(gwindow *window)
{
	gevent *event = NULL;
	if(!list_empty(&window->equeue)) {
		event = list_first_entry(&window->equeue, gui_event_s, list);
		list_del(&event->list);
	}
	return event;
}
