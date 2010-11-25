/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "lcd.h"
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common/bitops.h"
#include "lpt.h"

#ifdef CONFIG_FONT_TNR08X16
#include "ascii8x16.h"
#else
#include "ascii16x32.h"
#endif

struct list_head lcd_devs = LIST_HEAD_INIT(lcd_devs);

int lcd_add(const struct lcd_dev_s *dev, const char *name, int type)
{
	struct lcd_s *lcd = sys_malloc(sizeof(struct lcd_s));
	lcd -> dev = dev;
	lcd -> name = name;
	lcd -> type = 0;
	lcd -> font = ascii_16x32;
	lcd -> fgcolor = LCD_FGCOLOR_DEF;
	lcd -> bgcolor = LCD_BGCOLOR_DEF;
	lcd -> xres = 0;
	lcd -> yres = 0;
	lcd -> rot = LCD_ROT_DEF;
	list_add(&lcd -> list, &lcd_devs);

#ifdef CONFIG_FONT_TNR08X16
	lcd -> font = ascii_8x16;
#endif
	//lcd type
	if(type & LCD_TYPE_CHAR)
		lcd -> type |= LCD_TYPE_CHAR;

	//get real resolution
	lcd_get_res(lcd, &lcd -> xres, &lcd -> yres);
	return 0;
}

struct lcd_s *lcd_get(const char *name)
{
	struct list_head *pos;
	struct lcd_s *lcd;

	list_for_each(pos, &lcd_devs) {
		lcd = list_entry(pos, lcd_s, list);
		if( name == NULL ) {
			//default to the first one
			break;
		}

		if( !strcmp(lcd -> name, name) )
			break;
	}

	return lcd;
}

const char *lcd_get_font(struct lcd_s *lcd, int *w, int *h)
{
	const char *p = lcd -> font;
	*w = (int) p[0];
	*h = (int) p[1];
	return &p[2];
}

int lcd_get_res(struct lcd_s *lcd, int *x, int *y)
{
	int w, h;

	if(lcd -> xres) {
		*x = lcd -> xres;
		*y = lcd -> yres;
		return 0;
	}

	//to do calculation at the first time
	lcd_get_font(lcd, &w, &h);
	if(LCD_TYPE(lcd) == LCD_TYPE_CHAR) {
		*x = lcd -> dev -> xres * w;
		*y = lcd -> dev -> yres * h;
		return 0;
	}

	if(lcd -> rot == LCD_ROT_090 || lcd -> rot == LCD_ROT_270) {
		*x = lcd -> dev -> yres;
		*y = lcd -> dev -> xres;
	}
	else {
		*x = lcd -> dev -> xres;
		*y = lcd -> dev -> yres;
	}
	return 0;
}

int lcd_set_fgcolor(struct lcd_s *lcd, int cr)
{
	lcd -> fgcolor = cr;
	return 0;
}

int lcd_set_bgcolor(struct lcd_s *lcd, int cr)
{
	lcd -> bgcolor = cr;
	return 0;
}

int lcd_init(struct lcd_s *lcd)
{
	struct lcd_cfg_s cfg;
	cfg.rot = lcd -> rot;
	cfg.bus = &lpt;
	int ret = lcd -> dev -> init(&cfg);
	if( !ret ) {
		ret = lcd_clear_all(lcd);
	}
	return ret;
}

/*coordinate transformation, virtual -> real*/
static void lcd_transform(struct lcd_s *lcd, int *px, int *py)
{
	int x, y, w, h;

	w = lcd -> dev -> xres;
	h = lcd -> dev -> yres;

	switch (lcd -> rot) {
	case LCD_ROT_090:
		x = (*py);
		y = h - (*px) - 1;
		break;
	case LCD_ROT_180:
		x = w - (*px) - 1;
		y = h - (*py) - 1;
		break;
	case LCD_ROT_270:
		x = w - (*py) - 1;
		y = (*px);
		break;
	default:
		x = *px;
		y = *py;
	}

	//write back
	*px = x;
	*py = y;
}

static int lcd_set_window(struct lcd_s *lcd, int x, int y, int w, int h)
{
	int x0, y0, x1, y1;

	x0 = x;
	y0 = y;
	x1 = x + w - 1;
	y1 = y + h - 1;
	if(x0 < 0 || y0 < 0 || x1 >= lcd -> xres || y1 >= lcd -> yres) {
		return -1;
	}

	//virtual -> real
	lcd_transform( lcd, &x0, &y0 );
	lcd_transform( lcd, &x1, &y1 );
	return lcd -> dev -> setwindow( x0, y0, x1, y1 );
}

int lcd_bitblt(struct lcd_s *lcd, const void *bits, int x, int y, int w, int h)
{
	int i, n, v;
	int ret = lcd_set_window(lcd, x, y, w, h);;

	i = 0;
	n = w * h;
	while ( !ret && i < n ) {
		v = bit_get(i, bits);
		v = (v != 0) ? lcd -> fgcolor : lcd -> bgcolor;
		//warnning!!! be carefull of the endianess and rgb format here ...
		ret = lcd -> dev -> wgram(&v, 1);
		i ++;
	}

	return ret;
}

int lcd_imageblt(struct lcd_s *lcd, const void *image, int x, int y, int w, int h)
{
	int ret = lcd_set_window(lcd, x, y, w, h);
	if(!ret) {
		//warnning!!! be carefull of the rgb format here ...
		ret = lcd -> dev -> wgram(image, w * h);
	}

	return ret;
}

static int lcd_clear_pixel(struct lcd_s *lcd, int x, int y, int w, int h)
{
	int i, n;
	int ret = lcd_set_window(lcd, x, y, w, h);

	i = 0;
	n = w * h;
	while ( !ret && i < n ) {
		ret = lcd -> dev -> wgram(&lcd -> bgcolor, 1);
		i ++;
	}

	return ret;
}

static int lcd_clear_char(struct lcd_s *lcd, int x, int y, int w, int h)
{
	int ret = -1;
	int i;
	char *str;

	str = sys_malloc(w + 1);
	memset(str, ' ', w);
	str[w] = 0;

	for(i = 0; i < h; i++) {
		ret = lcd -> dev -> puts(x, y + i, str);
		if(ret)
			break;
	}

	sys_free(str);
	return ret;
}

int lcd_clear(struct lcd_s *lcd, int x, int y, int w, int h)
{
	int ret, fw, fh;
	lcd_get_font(lcd, &fw, &fh);
	if(LCD_TYPE(lcd) == LCD_TYPE_CHAR) {
		x /= fw;
		w /= fw;
		y /= fh;
		h /= fh;
		ret = lcd_clear_char(lcd, x, y, w, h);
	}
	else {
		ret = lcd_clear_pixel(lcd, x, y, w, h);
	}

	return ret;
}

int lcd_clear_all(struct lcd_s *lcd)
{
	int xres, yres;
	lcd_get_res(lcd, &xres, &yres);
	return lcd_clear(lcd, 0, 0, xres, yres);
}

/*pixel based char output*/
static int lcd_putchar_pixel(struct lcd_s *lcd, int x, int y, char c)
{
	int w, h;
	const char *font = lcd_get_font(lcd, &w, &h);

	font += (c - '!' + 1) << 6;
	return lcd_bitblt(lcd, font, x, y, w, h);
}

int lcd_puts(struct lcd_s *lcd, int x, int y, const char *str)
{
	int ret;
	int w, h;

	lcd_get_font(lcd, &w, &h);

	//char based lcd module
	if(LCD_TYPE(lcd) == LCD_TYPE_CHAR) {
		//convert x, y to char based coordinate system
		x /= w;
		y /= h;
		return lcd -> dev -> puts(x, y, str);
	}

	//pixel based lcd module
	while (*str) {
		ret = lcd_putchar_pixel(lcd, x, y, *str);
		if(ret)
			break;
		str ++ ;
		x += w;
	}

	return ret;
}
