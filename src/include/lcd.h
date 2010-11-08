/*
 * 	miaofng@2010 initial version
 */
#ifndef __LCD_H_
#define __LCD_H_

#include "config.h"
#include "common/glib.h"

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

#define COLOR_BG_DEF	WHITE
#define COLOR_FG_DEF	BLACK

typedef struct {
	int w; //width
	int h; //height
	int (*init)(void);
	int (*puts)(int x, int y, const char *str);
	int (*clear_all)(void); //opt
	int (*clear_rect)(int x, int y, int w, int h); //opt
	int (*scroll)(int xoffset, int yoffset); //opt
	int (*set_color)(int fg, int bg);
	int (*bitblt)(const void *src, int x, int y, int w, int h);
	
	int (*writereg)(int reg, int val);
	int (*readreg)(int reg);
} lcd_t;

int lcd_init(void);
int lcd_add(const lcd_t *dev);
int lcd_puts(int x, int y, const char *str);
int lcd_clear_all(void);
int lcd_clear_rect(int x, int y, int w, int h);
int lcd_scroll(int xoffset, int yoffset);
int lcd_is_visible(const rect_t *r);
int lcd_set_color(int fg, int bg);
int lcd_bitblt(const void *src, int x, int y, int w, int h);

//debug purpose
int lcd_writereg(int reg, int val);
int lcd_readreg(int reg);

//private
/*coordinate transformation, virtual -> real*/
static inline void lcd_transform(int *px, int *py, int w, int h)
{
	int x, y;

	w --;
	h --;
	
#if CONFIG_LCD_ROT_090 == 1
	x = (*py);
	y = h - (*px);
#elif CONFIG_LCD_ROT_180 == 1
	x = w - (*px);
	y = h - (*py);
#elif CONFIG_LCD_ROT_270 == 1
	x = w - (*py);
	y = (*px);
#else //CONFIG_LCD_ROT_000
	x = *px;
	y = *py;
#endif

	//write back
	*px = x;
	*py = y;
}

static inline void lcd_sort(int *p0, int *p1)
{
	int v;
	if(*p0 > *p1) {
		v = *p0;
		*p0 = *p1;
		*p1 = v;
	}
}

#endif /*__LCD_H_*/
