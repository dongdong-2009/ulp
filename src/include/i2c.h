/*
*	dusk@2010 initial version
*/
#ifndef __I2C_H_
#define __I2C_H_

#include <stddef.h>
#include "config.h"

typedef struct {
	int speed;			/*i2c speed,100K -> 100000*/
	int option;		/*just for more config for i2c*/
} i2c_cfg_t;

typedef struct {
	int (*init)(const i2c_cfg_t *cfg);
	int (*wreg)(char chip, int addr, int alen, const void *buffer);
	int (*rreg)(char chip, int addr, int alen, void *buffer);
	int (*wbuf)(char chip, int addr, int alen, const void *buffer, int len);
	int (*rbuf)(char chip, int addr, int alen, void *buffer, int len);
	int (*WaitStandByState)(char chip);
} i2c_bus_t;

extern i2c_bus_t i2c1;
extern i2c_bus_t i2c2;
extern i2c_bus_t i2cs;

#endif /*__I2C_H_*/
