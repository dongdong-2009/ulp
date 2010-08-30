/*
*	This bus driver is used to emulate a virtual parallel port to connect lcd or some other device
*
*	miaofng@2010 initial version
*/
#ifndef __LPT_H_
#define __LPT_H_

#include <stddef.h>
#include "config.h"

typedef struct {
	int (*init)(void);
	int (*write)(int addr, int val);
	int (*read)(int addr);
} lpt_bus_t;

extern lpt_bus_t lpt;

#endif /*__LPT_H_*/

