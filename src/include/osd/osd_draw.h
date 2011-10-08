/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_DRAW_H_
#define __OSD_DRAW_H_

#include "osd/osd_item.h"

extern widget_t widget_text;
extern widget_t widget_int;
extern widget_t widget_hex;

//item draw options
#define ITEM_DRAW_TXT	(&widget_text)
#define ITEM_DRAW_INT	(&widget_int)
#define ITEM_DRAW_HEX (&widget_hex)

#endif /*__OSD_DRAW_H_*/
