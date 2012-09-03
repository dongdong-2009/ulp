/*
 * 	miaofng@2012 initial version
 */
#ifndef __GUI_H_
#define __GUI_H_

#include "gui/gui_config.h"
#include "gui/gui_lcd.h"
#include "gui/gui_event.h"
#include "gui/gui_widget.h"

int gui_init(const char *cmdline);
int gui_update(void);

#endif /*__GUI_H_*/
