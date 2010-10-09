/*
*	common pointing device like touch screen/mouse driver header file
 * 	miaofng@2010 initial version
 */
#ifndef __PD_H_
#define __PD_H_

#include "config.h"
#include "common/glib.h"

//supported event
enum {
	PDE_NONE,
	PDE_DN,
	PDE_UP,
	PDE_CLICK,
	PDE_HOLD,
	PDE_DRAG,
};

int pd_Init(void);
int pd_SetMargin(const rect_t *r);
int pd_GetEvent(dot_t *p);

#endif /*__PD_H_*/
