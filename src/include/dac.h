/*
 *  david@2010 initial version
 */

#ifndef __DAC_H_
#define __DAC_H_

#include "config.h"

typedef struct {
	int option;
} dac_cfg_t;

typedef struct {
	int (*init)(const dac_cfg_t *cfg);
	int (*write)(int data);
} dac_t;

extern const dac_t dac_ch1;
extern const dac_t dac_ch2;

#endif /* __DAC_H_ */
