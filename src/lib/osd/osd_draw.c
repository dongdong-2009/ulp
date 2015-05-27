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
#include "sys/sys.h"

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
	buf = sys_malloc(len + 1);
	strncpy(buf, str, len);
	buf[len] = 0;
	
	//align
	x = item->x;
	x += (item->option & ITEM_ALIGN_MIDDLE) ? (item->w - len) >> 1 : 0;
	x += (item->option & ITEM_ALIGN_RIGHT) ? (item->w - len) : 0;
	
	//output to lcd
	osd_eng_puts(x, item->y, buf);

	sys_free(buf);
	return 0;
}

widget_t widget_text = {
	.draw = item_DrawTxt,
#ifdef CONFIG_DRIVER_PD
	.react = NULL,
#endif
};

int item_DrawInt(const osd_item_t *item, int status)
{
	int value;
	char *buf, *buf2;
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
	buf = sys_malloc(len + len);
	buf2 = buf + len;
	snprintf(buf, len, "%d", value);
	len = strlen(buf);
	
	//align
	//x = item->x;
	//x += (item->option & ITEM_ALIGN_MIDDLE) ? (item->w - len) >> 1 : 0;
	//x += (item->option & ITEM_ALIGN_RIGHT) ? (item->w - len) : 0;
	x = (item->option & ITEM_ALIGN_MIDDLE) ? (item->w - len) >> 1 : 0;
	x = (item->option & ITEM_ALIGN_RIGHT) ? (item->w - len) : 0;

	memset(buf2, ' ', item -> w);
	strcpy(buf2 + x, buf);
	
	//output to lcd
	//osd_eng_puts(x, item->y, buf);
	osd_eng_puts(item -> x, item -> y, buf2);

	sys_free(buf);
	return 0;
}

widget_t widget_int = {
	.draw = item_DrawInt,
#ifdef CONFIG_DRIVER_PD
	.react = NULL,
#endif
};


