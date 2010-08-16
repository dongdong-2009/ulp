/*
 * 	miaofng@2010 initial version
 */
#ifndef __LCD_H_
#define __LCD_H_

#include "config.h"

#define RGB565(r, g, b)	((r & 0x1f) | ((g & 0x3f) << 5) | ((b & 0x1f) << 11))
#define RGB RGB565

#define WHITE RGB(0xff, 0xff, 0xff)
#define BLACK RGB(0x00, 0x00, 0x00)
#define RED RGB(0xff, 0x00, 0x00)
#define GREEN RGB(0x00, 0xff, 0x00)
#define BLUE RGB(0x00, 0x00, 0xff)
#define YELLOW RGB(0xff, 0xff, 0x00)
#define PURPLE RGB(0xff, 0x00, 0xff)
#define CYAN RGB(0x00, 0xff, 0xff)

typedef struct {
	int w; //width
	int h; //height
	int (*init)(void);
	int (*puts)(int x, int y, const char *str);
	int (*clear_all)(void); //opt
	int (*clear_rect)(int x, int y, int w, int h); //opt
	int (*scroll)(int xoffset, int yoffset); //opt
	int (*set_color)(int fg, int bg);
	
	int (*writereg)(int reg, int val);
	int (*readreg)(int reg);
} lcd_t;

int lcd_init(void);
int lcd_add(const lcd_t *dev);
int lcd_puts(int x, int y, const char *str);
int lcd_clear_all(void);
int lcd_clear_rect(int x, int y, int w, int h);
int lcd_scroll(int xoffset, int yoffset);
int lcd_is_visible(int x, int y);
int lcd_set_color(int fg, int bg);

//debug purpose
int lcd_writereg(int reg, int val);
int lcd_readreg(int reg);

#endif /*__LCD_H_*/
