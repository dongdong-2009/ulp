/*
 * 	miaofng@2010 initial version
 */
#ifndef __LCD_H_
#define __LCD_H_

#define RGB565(r, g, b)	((unsigned short)(r | (g << 5) | (b << 11)))

typedef struct {
	int w; //width
	int h; //height
	int (*init)(void);
	int (*puts)(int x, int y, const char *str);
	int (*clear_all)(void); //opt
	int (*clear_rect)(int x, int y, int w, int h); //opt
	int (*scroll)(int xoffset, int yoffset); //opt
} lcd_t;

int lcd_init(void);
int lcd_add(const lcd_t *dev);
int lcd_puts(int x, int y, const char *str);
int lcd_clear_all(void);
int lcd_clear_rect(int x, int y, int w, int h);
int lcd_scroll(int xoffset, int yoffset);

#endif /*__LCD_H_*/
