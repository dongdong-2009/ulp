/*
 * 	miaofng@2012 initial version
 */
#ifndef __GUI_EVENT_H_
#define __GUI_EVENT_H_

#include "linux/list.h"
#include "gui/gui_widget.h"
#include "gui/gui_window.h"

typedef enum
{
	GUI_NOTHING		= -1,
	GUI_DELETE		= 0,
	GUI_DESTROY		= 1,
	GUI_EXPOSE		= 2,
	GUI_MOTION_NOTIFY	= 3,
	GUI_BUTTON_PRESS	= 4,
	GUI_2BUTTON_PRESS	= 5,
	GUI_DOUBLE_BUTTON_PRESS = GUI_2BUTTON_PRESS,
	GUI_3BUTTON_PRESS	= 6,
	GUI_TRIPLE_BUTTON_PRESS = GUI_3BUTTON_PRESS,
	GUI_BUTTON_RELEASE	= 7,
	GUI_KEY_PRESS		= 8,
	GUI_KEY_RELEASE		= 9,
	GUI_ENTER_NOTIFY	= 10,
	GUI_LEAVE_NOTIFY	= 11,
	GUI_FOCUS_CHANGE	= 12,
	GUI_CONFIGURE		= 13,
	GUI_MAP			= 14,
	GUI_UNMAP		= 15,
	GUI_PROPERTY_NOTIFY	= 16,
	GUI_SELECTION_CLEAR	= 17,
	GUI_SELECTION_REQUEST 	= 18,
	GUI_SELECTION_NOTIFY	= 19,
	GUI_PROXIMITY_IN	= 20,
	GUI_PROXIMITY_OUT	= 21,
	GUI_DRAG_ENTER		= 22,
	GUI_DRAG_LEAVE		= 23,
	GUI_DRAG_MOTION		= 24,
	GUI_DRAG_STATUS		= 25,
	GUI_DROP_START		= 26,
	GUI_DROP_FINISHED	= 27,
	GUI_CLIENT_EVENT	= 28,
	GUI_VISIBILITY_NOTIFY	= 29,
	GUI_SCROLL		= 31,
	GUI_WINDOW_STATE	= 32,
	GUI_SETTING		= 33,
	GUI_OWNER_CHANGE	= 34,
	GUI_GRAB_BROKEN		= 35,
	GUI_DAMAGE		= 36,
	GUI_TOUCH_BEGIN		= 37,
	GUI_TOUCH_UPDATE	= 38,
	GUI_TOUCH_END		= 39,
	GUI_TOUCH_CANCEL	= 40,
	GUI_EVENT_LAST        /* helper variable for decls */
} gevent_type;

struct gui_event_s {
	gevent_type type;
	gwidget *dst;
	union {
		struct {
			unsigned x: 12;
			unsigned y: 12;
			unsigned z: 8; /*pressure*/
		} ts;
	};
	struct list_head list;
};

void gui_event_init(void);
void gui_event_update(void);
gevent* gui_event_new(gevent_type type, gwidget *dst);
void gui_event_del(gevent *event);
int gui_event_connect(gwidget *widget, gcallback func);
int gui_event_emit(gwindow *window, gevent *event);
gevent* gui_event_recv(gwindow *window);

#endif /*__GUI_EVENT_H_*/
