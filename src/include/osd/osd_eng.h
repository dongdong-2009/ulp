/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_ENG_H_
#define __OSD_ENG_H_

#include "common/glib.h"

int osd_eng_init(void);
int osd_eng_puts(int x, int y, const char *str);
int osd_eng_clear_all(void);
int osd_eng_clear_rect(int x, int y, int w, int h);
int osd_eng_scroll(int xoffset, int yoffset);
int osd_eng_set_color(int fg, int bg);
int osd_eng_is_visible(const rect_t *r);

#endif /*__OSD_ENG_H_*/
