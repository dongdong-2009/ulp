/* item.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_ITEM_H_
#define __OSD_ITEM_H_

#include "config.h"
#include "common/glib.h"

//update options
enum {
	ITEM_UPDATE_NEVER = 0,
	//ITEM_UPDATE_ROUNDROBIN,
	ITEM_UPDATE_AFTERCOMMAND,
	ITEM_UPDATE_AFTERFOCUSCHANGE,
	ITEM_UPDATE_ALWAYS,
};

//runtime options
enum {
	ITEM_RUNTIME_NONE = 0,
	ITEM_RUNTIME_V = 1 << 4,
};

//misc options
enum {
	//align options
	ITEM_ALIGN_LEFT = 0,
	ITEM_ALIGN_MIDDLE,
	ITEM_ALIGN_RIGHT,
};

//widget_t
struct osd_item_s;

typedef struct {
	int (*draw)(const struct osd_item_s *item, int status); //draw function
#ifdef CONFIG_DRIVER_PD
	int (*react)(const struct osd_item_s *item, int event, const dot_t *p);
#endif
} widget_t;

typedef struct osd_item_s {
	char x; //hor position
	char y; //vert position
	char w; //width
	char h; //height
	int v; //value
	widget_t *draw; //widget type, could be ITEM_DRAW_TEXT, ...
	int option; //misc draw options, refer to misc options
	short update; //refer to update options 
	short runtime; //refer to runtime options
} osd_item_t;

//private
int osd_ShowItem(const osd_item_t *item, int status);
int osd_HideItem(const osd_item_t *item);
#ifdef CONFIG_DRIVER_PD
int osd_item_react(const osd_item_t *item, int event, const dot_t *p);
#endif

#endif /*__OSD_ITEM_H_*/
