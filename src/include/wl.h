/*
 *  miaofng@2010 initial version
 */

#ifndef __WL_H_
#define __WL_H_

#include "config.h"

typedef struct {
	int mhz; //unit: MHz, max 4.3Ghz
	int bps; //unit: bps
} wl_cfg_t;

#define WL_CFG_DEF { \
	.mhz = 24000 + 30, \
	.bps = 2000000, \
}

typedef struct {
	int (*init)(const wl_cfg_t *cfg);
	int (*send)(const char *buf, int count, int ms);// return n bytes sent
	int (*recv)(char *buf, int count, int ms); // return n bytes received, no more than count bytes, or error code

	int (*poll)(void); //return 0 in case rx fifo is empty
	int (*select)(int target); //used for multi device communication
} wl_bus_t;

extern const wl_bus_t wl0;

#endif /* __WL_H_ */
