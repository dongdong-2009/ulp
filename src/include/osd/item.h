/* item.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_ITEM_H_
#define __OSD_ITEM_H_

#include "osd/osd_eng.h"

#define COLOR_FG_DEF BLACK
#define COLOR_BG_DEF WHITE	

#define COLOR_FG_FOCUS WHITE
#define COLOR_BG_FOCUS BLUE

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
//	ITEM_RUNTIME_X = 1 << 0,
//	ITEM_RUNTIME_Y = 1 << 1,
//	ITEM_RUNTIME_W = 1 << 2,
//	ITEM_RUNTIME_H = 1 << 3,
	ITEM_RUNTIME_V = 1 << 4,
};

//misc options
enum {
	//align options
	ITEM_ALIGN_LEFT = 0,
	ITEM_ALIGN_MIDDLE,
	ITEM_ALIGN_RIGHT,
};

//item/group status
enum {
	STATUS_HIDE = -2,
	STATUS_VISIBLE,
	STATUS_GRAYED,
	STATUS_NORMAL,
	STATUS_FOCUSED,
};

typedef struct osd_item_s {
	int x; //hor position
	int y; //vert position
	int w; //width
	int h; //height
	int v; //value
	int (*draw)(const struct osd_item_s *item, int status); //draw function
	int option; //misc draw options, refer to misc options
	short update; //refer to update options 
	short runtime; //refer to runtime options
} osd_item_t;

int item_DrawTxt(const osd_item_t *item, int status);
int item_DrawInt(const osd_item_t *item, int status);
#endif /*__OSD_ITEM_H_*/
