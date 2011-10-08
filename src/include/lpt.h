/*
*	This bus driver is used to emulate a virtual parallel port to connect lcd or some other device
*
*	miaofng@2010 initial version
*/
#ifndef __LPT_H_
#define __LPT_H_

#include <stddef.h>
#include "config.h"

//bus mode
enum {
	LPT_MODE_I80, //intel i80 timming i/f
	LPT_MODE_M68, //m68 timing i/f
};

struct lpt_cfg_s {
	int mode; //bus mode
	int tp; //bus access time
	int t; //bus period
};

typedef struct {
	int (*init)(const struct lpt_cfg_s *cfg);
	int (*write)(int addr, int val);
	int (*read)(int addr);
	int (*writeb)(int addr, const void *buf, int n); //buffer write
	int (*writen)(int addr, int v, int n); //repeated write the same data
} lpt_bus_t;

#define LPT_CFG_DEF { \
	.mode = LPT_MODE_I80, \
	.tp = CONFIG_LPT_TP, \
	.t = CONFIG_LPT_T, \
}

extern const lpt_bus_t lpt;

#endif /*__LPT_H_*/

