/*
 * 	miaofng@2012 initial version
 */
#ifndef __GUI_LCD_H_
#define __GUI_LCD_H_

int gui_lcd_init(void);
void gui_lcd_pen(int fgcolor, int bgcolor, int width);
void gui_lcd_get_resolution(int *xres, int *yres);
void gui_lcd_line(int x, int y, int dx, int dy);
void gui_lcd_box(int x, int y, int w, int h);
void gui_lcd_rectangle(int x, int y, int w, int h);
void gui_lcd_clear(int x, int y, int w, int h);
void gui_lcd_puts(int x, int y, const char *text);
void gui_lcd_get_font(int *fw, int *fh);
void gui_lcd_imageblt(const void *image, int x, int y, int w, int h);

#endif /*__GUI_LCD_H_*/
