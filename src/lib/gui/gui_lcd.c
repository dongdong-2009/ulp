/*
 * 	miaofng@2012 initial version
 *
 */

#include "gui/gui_config.h"
#include "gui/gui_lcd.h"
#include "gui/gui_event.h"
#include "lcd.h"
#include <stdlib.h>

static struct lcd_s *gui_lcd = NULL;

int gui_lcd_init(void)
{
	gui_lcd = lcd_get(NULL);
	return lcd_init(gui_lcd);
}

void gui_lcd_get_resolution(int *xres, int *yres)
{
	lcd_get_res(gui_lcd, xres, yres);
}

void gui_lcd_pen(int fgcolor, int bgcolor, int width)
{
	lcd_set_fgcolor(gui_lcd, fgcolor);
	lcd_set_bgcolor(gui_lcd, bgcolor);
	lcd_set_line_width(gui_lcd, width);
}

void gui_lcd_line(int x, int y, int dx, int dy)
{
	lcd_line(gui_lcd, x, y, dx, dy);
}

void gui_lcd_box(int x, int y, int w, int h)
{
	lcd_box(gui_lcd, x, y, w, h);
}

void gui_lcd_rectangle(int x, int y, int w, int h)
{
	lcd_rect(gui_lcd, x, y, w, h);
}

void gui_lcd_clear(int x, int y, int w, int h)
{
	lcd_clear(gui_lcd, x, y, w, h);
}

void gui_lcd_puts(int x, int y, const char *text)
{
	lcd_puts(gui_lcd, x, y, text);
}

void gui_lcd_get_font(int *fw, int *fh)
{
	lcd_get_font(gui_lcd, fw, fh);
}

void gui_lcd_imageblt(const void *image, int x, int y, int w, int h)
{
	lcd_imageblt(gui_lcd, image, x, y, w, h);
}
