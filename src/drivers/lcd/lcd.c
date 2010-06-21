/*
 * 	miaofng@2010 initial version
 *		-virtual clear_all/clear_rect/scroll support
 */

#include "config.h"
#include "lcd.h"
#include <stdlib.h>
#include <string.h>

static const lcd_t *lcd;
static int lcd_xoffset;
static int lcd_yoffset;

int lcd_add(const lcd_t *dev)
{
	lcd = dev;
	lcd->init();
	return 0;
}

int lcd_init(void)
{
	int ret = -1;

	lcd_xoffset = 0;
	lcd_yoffset = 0;

	if(lcd != NULL)
		ret = lcd->init();

	return ret;
}

int lcd_puts(int x, int y, const char *str)
{
	int ret = -1;

	x -= lcd_xoffset;
	y -= lcd_yoffset;

	if((y >= lcd->h) || (x >= lcd->w) || (y < 0) || (x < 0)) {
		return ret;
	}

	ret = lcd->puts(x, y, str);
	return ret;
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
	str = malloc(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = lcd->puts(0, i, str);
		if(ret)
			break;
	}

	free(str);
	return ret;
}

int lcd_clear_rect(int x, int y, int w, int h)
{
	int ret = -1;
	int i;
	char *str;

	x -= lcd_xoffset;
	y -= lcd_yoffset;

	if(lcd->clear_rect != NULL)
		return lcd->clear_rect(x, y, w, h);

	if((y >= lcd->h) || (x >= lcd->w) || (y < 0) || (x < 0)) {
		return ret;
	}

	str = malloc(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = lcd->puts(x, y + i, str);
		if(ret)
			break;
	}

	free(str);
	return ret;
}

int lcd_scroll(int xoffset, int yoffset)
{
	int ret = -1;

	if((xoffset < 0) && (yoffset < 0))
		return (int)(lcd->scroll == NULL);

	if(lcd->scroll != NULL)
		ret = lcd->scroll(xoffset, yoffset);
	else {
		lcd_xoffset = xoffset;
		lcd_yoffset = yoffset;
		ret = 0;
	}

	return ret;
}


