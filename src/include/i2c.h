/*
*	dusk@2010 initial version
*/
#ifndef __I2C_H_
#define __I2C_H_

#include <stddef.h>
#include "device.h"
#include "config.h"

typedef struct {
	unsigned speed;			/*i2c speed,100K -> 100000*/
	unsigned option;		/*just for more config for i2c*/
} i2c_cfg_t;

typedef struct {
	int (*init)(const i2c_cfg_t *cfg);
	int (*wreg)(unsigned char chip, unsigned addr, int alen, unsigned char *buffer);
	int (*rreg)(unsigned char chip, unsigned addr, int alen, unsigned char *buffer);
	int (*wbuf)(unsigned char chip, unsigned addr, int alen, unsigned char *buffer, int len);
	int (*rbuf)(unsigned char chip, unsigned addr, int alen, unsigned char *buffer, int len);
	int (*WaitStandByState)(unsigned char chip);
} i2c_bus_t;

extern i2c_bus_t i2c1;
extern i2c_bus_t i2c2;

#endif /*__I2C_H_*/
