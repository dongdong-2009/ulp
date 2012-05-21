/*
*	david@2012 initial version
*/
#ifndef __COUNTER_H_
#define __COUNTER_H_

#include <stddef.h>
#include "config.h"

typedef struct {
	int option; //counter config, option for future use
} counter_cfg_t;

typedef struct {
	void (*init)(const counter_cfg_t *cfg);
	int (*get)(void);
	void (*set)(int value);
} counter_bus_t;

#define COUNTER_CFG_DEF { \
	.option = 0, \
}

#ifdef CONFIG_DRIVER_COUNTER0
extern const counter_bus_t counter0;
extern const counter_bus_t counter01;
extern const counter_bus_t counter02;
extern const counter_bus_t counter03;
extern const counter_bus_t counter04;
#endif

#ifdef CONFIG_DRIVER_COUNTER1
extern const counter_bus_t counter1;
extern const counter_bus_t counter11;
extern const counter_bus_t counter12;
extern const counter_bus_t counter13;
extern const counter_bus_t counter14;
#endif

#ifdef CONFIG_DRIVER_COUNTER2
extern const counter_bus_t counter2;
extern const counter_bus_t counter21;
extern const counter_bus_t counter22;
extern const counter_bus_t counter23;
extern const counter_bus_t counter24;
#endif

#ifdef CONFIG_DRIVER_COUNTER3
extern const counter_bus_t counter3;
extern const counter_bus_t counter31;
extern const counter_bus_t counter32;
extern const counter_bus_t counter33;
extern const counter_bus_t counter34;
#endif

#ifdef CONFIG_DRIVER_COUNTER3
extern const counter_bus_t counter4;
extern const counter_bus_t counter41;
extern const counter_bus_t counter42;
extern const counter_bus_t counter43;
extern const counter_bus_t counter44;
#endif

#endif /*__COUNTER_H_*/

