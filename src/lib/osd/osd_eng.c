/*
 * 	miaofng@2010 initial version
 *		- separate os_eng op from general lcd driver, this file provide char based osd engine
 */

#include "config.h"
#include "osd/osd_eng.h"
#include "FreeRTOS.h"
#include <stdlib.h>
#include "lcd.h"

static struct lcd_s *osd_eng_lcd = NULL;
static char osd_eng_xofs;
static char osd_eng_yofs;

int osd_eng_init(void)
{
	osd_eng_xofs = 0;
	osd_eng_yofs = 0;
	osd_eng_lcd = lcd_get(NULL);
	return lcd_init(osd_eng_lcd);
}

int osd_eng_puts(int x, int y, const char *str)
{
	int fw, fh;
	lcd_get_font(osd_eng_lcd, &fw, &fh);
	x -= osd_eng_xofs;
	y -= osd_eng_yofs;
	x *= fw;
	y *= fh;
	return lcd_puts(osd_eng_lcd, x, y, str);
}

int osd_eng_clear_all(void)
{
	return lcd_clear_all(osd_eng_lcd);
}

int osd_eng_clear_rect(int x, int y, int w, int h)
{
	int fw, fh;
	lcd_get_font(osd_eng_lcd, &fw, &fh);
	x -= osd_eng_xofs;
	y -= osd_eng_yofs;
	x *= fw;
	w *= fw;
	y *= fh;
	h *= fh;
	return lcd_clear(osd_eng_lcd, x, y, w, h);
}

int osd_eng_scroll(int xoffset, int yoffset)
{
	osd_eng_xofs = xoffset;
	osd_eng_yofs = yoffset;
	return 0;
}

int osd_eng_set_color(int fg, int bg)
{
	lcd_set_fgcolor(osd_eng_lcd, fg);
	lcd_set_bgcolor(osd_eng_lcd, bg);
	return 0;
}

int osd_eng_is_visible(const rect_t *r)
{
	int xres, yres, fx, fy;
	rect_t rlcd;
	lcd_get_res(osd_eng_lcd, &xres, &yres);
	lcd_get_font(osd_eng_lcd, &fx, &fy);
	xres /= fx;
	yres /= fy;
	rect_set(&rlcd, osd_eng_xofs, osd_eng_yofs, xres, yres);
	return rect_have_rect(&rlcd, r);
}
