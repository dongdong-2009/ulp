/*
 * 	miaofng@2010 initial version
 *		-virtual clear_all/clear_rect/scroll support
 */

#include "config.h"
#include "osd/osd_eng.h"
#include "lcd1602.h"
#include <stdlib.h>
#include <string.h>

static const osd_engine_t osd_eng_lcd1602 = {
	.w = 16,
	.h = 2,
	.init = lcd1602_Init,
	.puts = lcd1602_WriteString,
	.clear_all = lcd1602_ClearScreen,
	.clear_rect = NULL,
	.scroll = NULL,
};

static const osd_engine_t *osd_engine;
static int osd_eng_xoffset;
static int osd_eng_yoffset;

int osd_eng_init(void)
{
	int ret = -1;

	osd_engine = &osd_eng_lcd1602;
	osd_eng_xoffset = 0;
	osd_eng_yoffset = 0;

	if(osd_engine->init != NULL)
		ret = osd_engine->init();

	return ret;
}

int osd_eng_puts(int x, int y, const char *str)
{
	int ret = -1;

	x -= osd_eng_xoffset;
	y -= osd_eng_yoffset;

	if((y >= osd_engine->h) || (x >= osd_engine->w) || (y < 0) || (x < 0)) {
		return ret;
	}

	ret = osd_engine->puts(x, y, str);
	return ret;
}

int osd_eng_clear_all(void)
{
	int ret = -1;
	int i, w, h;
	char *str;

	if(osd_engine->clear_all != NULL)
		return osd_engine->clear_all();

	w = osd_engine->w;
	h = osd_engine->h;
	str = malloc(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = osd_engine->puts(0, i, str);
		if(ret)
			break;
	}

	free(str);
	return ret;
}

int osd_eng_clear_rect(int x, int y, int w, int h)
{
	int ret = -1;
	int i;
	char *str;

	x -= osd_eng_xoffset;
	y -= osd_eng_yoffset;

	if(osd_engine->clear_rect != NULL)
		return osd_engine->clear_rect(x, y, w, h);

	if((y >= osd_engine->h) || (x >= osd_engine->w) || (y < 0) || (x < 0)) {
		return ret;
	}

	str = malloc(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = osd_engine->puts(x, y + i, str);
		if(ret)
			break;
	}

	free(str);
	return ret;
}

int osd_eng_scroll(int xoffset, int yoffset)
{
	int ret = -1;

	if((xoffset < 0) && (yoffset < 0))
		return (int)(osd_engine->scroll == NULL);

	if(osd_engine->scroll != NULL)
		ret = osd_engine->scroll(xoffset, yoffset);
	else {
		osd_eng_xoffset = xoffset;
		osd_eng_yoffset = yoffset;
		ret = 0;
	}

	return ret;
}


