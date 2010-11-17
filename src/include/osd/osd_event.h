/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_EVENT_H_
#define __OSD_EVENT_H_

#include <stddef.h>
#include "key.h"
#include "common/glib.h"

enum {
	OSDE_NONE = KEY_NONE,
	//...KEY EVENTS
	OSDE_DLG_QUIT = KEY_DUMMY,
	OSDE_GRP_FOCUS,
};

int osd_event_init(void);
int osd_event_get(void);

//return 0 if pd event outside the rect, note:
//para rc is char based coordinated system
//para pp is pixel based coordinated system
int osd_event_try(const rect_t *rc, const dot_t *pp);

#endif /*__OSD_EVENT _H_*/
