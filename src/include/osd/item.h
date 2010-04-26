/* item.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_ITEM_H_
#define __OSD_ITEM_H_

//update options
enum {
	ITEM_UPDATE_NEVER = 0,
	ITEM_UPDATE_ALWAYS,
	ITEM_UPDATE_ROUNDROBIN,
	ITEM_UPDATE_AFTERCOMMAND,
	ITEM_UPDATE_AFTERFOCUSCHANGE,
};

//runtime options
enum {
	ITEM_RUNTIME_X = 0,
	ITEM_RUNTIME_Y,
	ITEM_RUNTIME_W,
	ITEM_RUNTIME_H,
	ITEM_RUNTIME_V
};

//options
enum {
	//align options
	ITEM_ALIGN_LEFT = 0,
	ITEM_ALIGN_MIDDLE,
	ITEM_ALIGN_RIGHT,
};

typedef struct osd_item_s {
	int x; //hor position
	int y; //vert position
	int w; //width
	int h; //height
	int v; //value
	const int (*draw)(const struct osd_item_s *item); //draw function
	int option; //misc draw options
	short update; //refer to update options 
	short runtime; //refer to runtime options
} osd_item_t;

int item_DrawTxt(const osd_item_t *item);
int item_DrawInt(const osd_item_t *item);
#endif /*__OSD_ITEM_H_*/
