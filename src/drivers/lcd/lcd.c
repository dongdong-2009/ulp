/*
 * 	miaofng@2010 initial version
 *		-virtual clear_all/clear_rect/scroll support
 */

#include "config.h"
#include "lcd.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "common/glib.h"
#include "pd.h"

static const lcd_t *lcd;
static rect_t lcd_rect;

int lcd_add(const lcd_t *dev)
{
	lcd = dev;
	lcd->init();
	rect_set(&lcd_rect, 0, 0, lcd -> w, lcd -> h);
	return 0;
}

int lcd_init(void)
{
	int ret = -1;
	
	rect_zero(&lcd_rect);
	if(lcd != NULL) {
		rect_set(&lcd_rect, 0, 0, lcd -> w, lcd -> h);
		ret = lcd->init();
	}

	return ret;
}

int lcd_puts(int x, int y, const char *str)
{
	x -= lcd_rect.x1;
	y -= lcd_rect.y1;
	
	return lcd->puts(x, y, str);
}

int lcd_clear_all(void)
{
	int ret = -1;
	int i, w, h;
	char *str;

	if(lcd->clear_all != NULL)
		return lcd->clear_all();

	w = lcd->w;
	h = lcd->h;
	str = MALLOC(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = lcd->puts(0, i, str);
		if(ret)
			break;
	}

	FREE(str);
	return ret;
}

int lcd_clear_rect(int x, int y, int w, int h)
{
	int ret = -1;
	int i;
	char *str;

	x -= lcd_rect.x1;
	y -= lcd_rect.y1;

	if(lcd->clear_rect != NULL)
		return lcd->clear_rect(x, y, w, h);

	str = MALLOC(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = lcd->puts(x, y + i, str);
		if(ret)
			break;
	}

	FREE(str);
	return ret;
}

int lcd_scroll(int xoffset, int yoffset)
{
	int ret = -1;

	xoffset -= lcd_rect.x1;
	yoffset -= lcd_rect.y1;
	
	if(lcd->scroll != NULL)
		ret = lcd->scroll(xoffset, yoffset);
	else {
		rect_move(&lcd_rect, xoffset, yoffset);
		ret = 0;
	}

	return ret;
}

int lcd_is_visible(const rect_t *r)
{
	return rect_have_rect(&lcd_rect, r);
}

int lcd_set_color(int fg, int bg)
{
	if(lcd && lcd->set_color) {
		return lcd->set_color(fg, bg);
	}
	else
		return -1;
}

int lcd_bitblt(const void *src, int x, int y, int w, int h)
{
	if(lcd && lcd->bitblt) {
		return lcd->bitblt(src, x, y, w, h);
	}
	else
		return -1;
}

int lcd_get_prop(lcd_prop_t *prop)
{
	prop -> w = lcd -> w;
	prop -> h = lcd -> h;
	return 0;
}

int lcd_writereg(int reg, int val)
{
	if(lcd && lcd->writereg) {
		return lcd->writereg(reg, val);
	}
	else
		return -1;
}

int lcd_readreg(int reg)
{
	if(lcd && lcd->readreg) {
		return lcd->readreg(reg);
	}
	else
		return -1;
}
