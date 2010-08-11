/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_DRAW_H_
#define __OSD_DRAW_H_

#include "osd/osd_item.h"

//item draw options
#define ITEM_DRAW_TXT	item_DrawTxt
#define ITEM_DRAW_INT	item_DrawInt

int item_DrawTxt(const osd_item_t *item, int status);
int item_DrawInt(const osd_item_t *item, int status);

#endif /*__OSD_DRAW_H_*/
