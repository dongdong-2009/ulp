/*
 *	junjun@2011 initial version
 *
 */
#ifndef __DAC_H_
#define __DAC_H_

#include "spi.h"

/*IOCTL CMDS - there is no specific call sequency requirements*/
enum {
	DAC_GET_BITS,
	DAC_SET_CH, /*choose channel*/
};

/*for chip ad5663 16bit dac*/
struct ad5663_cfg_s {
	const spi_bus_t *spi;
	char gpio_cs;
	char gpio_ldac;
	char gpio_clr;
	char ch; //channel 0/1
};


#endif /*__DAC_H_*/
