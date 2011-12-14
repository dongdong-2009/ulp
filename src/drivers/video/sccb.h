/*
*	sccb bus
*
*	miaofng@2011 initial version
*/
#ifndef __SCCB_H_
#define __SCCB_H_

#include <stddef.h>
#include "config.h"

//bus mode
enum {
	SCCB_MODE_FIFO, //fifo mode
	SCCB_MODE_RAW, //direct mode, not supported yet
};

struct sccb_cfg_s {
	int mode; //bus mode
};

typedef struct {
	int (*init)(const struct sccb_cfg_s *cfg);
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

#endif /*__SCCB_H_*/

