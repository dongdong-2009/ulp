/*
*	common pointing device like touch screen/mouse driver header file
 * 	miaofng@2010 initial version
 */
#ifndef __PD_H_
#define __PD_H_

#include "config.h"
#include "common/glib.h"

#ifndef CONFIG_PD_RX
#define CONFIG_PD_RX 600 //unit; Ohm
#endif

#ifndef CONFIG_PD_DX
#define CONFIG_PD_DX	(12800)
#endif

#ifndef CONFIG_PD_DY
#define CONFIG_PD_DY	(12800)
#endif

#ifndef CONFIG_PD_ZL
#define CONFIG_PD_ZL	(3000)
#endif

//supported event
enum {
	PDE_NONE,
	PDE_DN,
	PDE_UP,
	PDE_CLICK,
	PDE_HOLD,
	PDE_DRAG,
};

//for bottom ts chip driver configuration
extern int pd_dx; //Delta-X limit
extern int pd_dy; //Delta-Y limit
extern int pd_zl; //RZ-limit

int pd_Init(void);
int pd_SetMargin(const rect_t *r);
int pd_GetEvent(dot_t *p);

struct pd_sample {
	int x;
	int y;
	int z; //force resistor, the bigger, the smaller, unit: Ohm
	//struct timeval tv;
};

/* get position and pressure info from bottom touch screen chip driver
* x, y pos info, 15bit
* return pressure resistor info, unit: Ohm
*/
int pdd_get(struct pd_sample *sp);

#endif /*__PD_H_*/
