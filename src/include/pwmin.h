/*
*	David@2010 initial version
*/
#ifndef __PWMIN_H_
#define __PWMIN_H_

#include <stddef.h>
#include "config.h"

//wave          ____|--------|______
//sample clock | | | | | | | | | | |

typedef struct {
	int sample_clock; //sample clock 
	int option;
} pwmin_cfg_t;

#define PWMIN_CFG_DEF { \
	.sample_clock = 1000000, \
	.option = 0, \
}

typedef struct {
	int (*init)(const pwmin_cfg_t *cfg);
	int (*getdc)(void);
	int (*getfrq)(void);
} pwmin_bus_t;

#ifdef CONFIG_DRIVER_PWMIN1
extern const pwmin_bus_t pwmin1;
extern const pwmin_bus_t pwmin11;
extern const pwmin_bus_t pwmin12;
#endif

#ifdef CONFIG_DRIVER_PWMIN2
extern const pwmin_bus_t pwmin2;
extern const pwmin_bus_t pwmin21;
extern const pwmin_bus_t pwmin22;
#endif

#ifdef CONFIG_DRIVER_PWMIN3
extern const pwmin_bus_t pwmin3;
extern const pwmin_bus_t pwmin31;
extern const pwmin_bus_t pwmin32;
#endif

#ifdef CONFIG_DRIVER_PWMIN4
extern const pwmin_bus_t pwmin4;
extern const pwmin_bus_t pwmin41;
extern const pwmin_bus_t pwmin42;
#endif

#endif /*__PWMIN_H_*/

