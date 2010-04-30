/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_ENG_H_
#define __OSD_ENG_H_

#include <stddef.h>

typedef struct {
	int w; //width
	int h; //height
	int (*init)(void);
	int (*puts)(int x, int y, const char *str);
	int (*clear_all)(void);
	int (*clear_rect)(int x, int y, int w, int h);
	int (*scroll)(int xoffset, int yoffset);
} osd_engine_t;

int osd_eng_init(void);
int osd_eng_puts(int x, int y, const char *str);
int osd_eng_clear_all(void);
int osd_eng_clear_rect(int x, int y, int w, int h);
int osd_eng_scroll(int xoffset, int yoffset);

#endif /*__OSD_ENG_H_*/
