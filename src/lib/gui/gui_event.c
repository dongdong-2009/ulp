/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_event.h"
#include "common/mempool.h"
#include "linux/list.h"
#include "key.h"
#include "pd.h"
#include "ulp_time.h"

static mempool_t *gui_epool;
static struct list_head gui_equeue_public;

void gui_event_init(void)
{
	gui_epool = mempool_create(sizeof(gevent), CONFIG_GUI_EPSZ);
	INIT_LIST_HEAD(&gui_equeue_public);

#ifdef CONFIG_DRIVER_PD
	pd_Init();
#endif
	key_Init();
}

void gui_event_update(void)
{
	gevent *event = NULL;
#ifdef CONFIG_DRIVER_PD
	unsigned x, y, z;
	gevent_type et = (gevent_type) pd_get_event(&x, &y, &z);
	if(et != GUI_NOTHING) {
		event = gui_event_new(et, NULL);
		event->ts.x = x;
		event->ts.y = y;
		event->ts.z = z;
		gui_event_emit(NULL, event);
	}
#endif
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

int gui_event_connect(gwidget *widget, gcallback func)
{
	widget->usr_event_func = func;
	return 0;
}

/*gwindow event queue*/
int gui_event_emit(gwindow *window, gevent *event)
{
	struct list_head *equeue;
	equeue = (window == NULL) ? &gui_equeue_public : &window->equeue;
	list_add_tail(&event->list, equeue);
	return 0;
}

gevent* gui_event_recv(gwindow *window)
{
	struct list_head *equeue;
	gevent *event = NULL;

	equeue = (window == NULL) ? &gui_equeue_public : &window->equeue;
	if(!list_empty(equeue)) {
		event = list_first_entry(equeue, gui_event_s, list);
		list_del(&event->list);
	}
	return event;
}
