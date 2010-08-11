/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd_draw.h"
#include "osd/osd_item.h"
#include "osd/osd_eng.h"
#include "osd/osd_group.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"

int item_SetColor(int status)
{
	int fg, bg;
	
	fg = (int) COLOR_FG_DEF;
	bg = (int) COLOR_BG_DEF;
	
	if(status == STATUS_FOCUSED) {
		fg = (int) COLOR_FG_FOCUS;
		bg = (int) COLOR_BG_FOCUS;
	}
	
	return osd_eng_set_color(fg, bg);
}

int item_DrawTxt(const osd_item_t *item, int status)
{
	const char *str;
	char *buf;
	size_t len;
	int x;
	int (*get_value)(void);
	
	//get string value
	str = (const char *) item->v;
	if(item->runtime & ITEM_RUNTIME_V) {
		get_value = (int (*)(void)) item->v;
		str = (const char *) get_value();
	}
	
	//string width limit
	len = strlen(str);
	len = (len > item->w) ? item->w : len;
	buf = MALLOC(len + 1);
	strncpy(buf, str, len);
	buf[len] = 0;
	
	//align
	x = item->x;
	x += (item->option & ITEM_ALIGN_MIDDLE) ? (item->w - len) >> 1 : 0;
	x += (item->option & ITEM_ALIGN_RIGHT) ? (item->w - len) : 0;
	
	//output to lcd
	item_SetColor(status);
	osd_eng_puts(x, item->y, buf);

	FREE(buf);
	return 0;
}

int item_DrawInt(const osd_item_t *item, int status)
{
	int value;
	char *buf;
	size_t len;
	int x;
	int (*get_value)(void);
	
	//get int value
	value = item->v;
	if(item->runtime & ITEM_RUNTIME_V) {
		get_value = (int (*)(void)) item->v;
		value = get_value();
	}
	
	//convert int to string & width limit
	len = item->w + 1;
	buf = MALLOC(len);
	snprintf(buf, len, "%d", value);
	len = strlen(buf);
	
	//align
	x = item->x;
	x += (item->option & ITEM_ALIGN_MIDDLE) ? (item->w - len) >> 1 : 0;
	x += (item->option & ITEM_ALIGN_RIGHT) ? (item->w - len) : 0;
	
	//output to lcd
	if(status == STATUS_FOCUSED)
		osd_eng_set_color(COLOR_FG_FOCUS, COLOR_BG_FOCUS);
	
	item_SetColor(status);
	osd_eng_puts(x, item->y, buf);
	
	FREE(buf);
	return 0;
}



