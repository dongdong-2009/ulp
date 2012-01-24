/*
 * 	junjun@2011 initial version
 */
#ifndef __AD5663_H_
#define __AD5663_H_

#include "ulp/device.h"

/*IOCTL CMDS - there is no specific call sequency requirements*/
enum {
	DAC_WRITEN,
	DAC_UPDATEN,
	DAC_WRIN_UPDALL,
	DAC_WRIN_UPDN,
	DAC_POWER_DW,
	DAC_RESET,
	DAC_REG_SET,
	DAC_RESV,
	DAC_CHOOSE_CHANNEL1, /*Channel 1 choose*/
	DAC_CHOOSE_CHANNEL2,
};

struct ad5663_chip_s {
	const spi_bus_t *spi;
	char gpio_cs;
	char gpio_ldac;
	char gpio_clr;
	struct list_head pipes;
};

struct ad5663_buffer_s {
	unsigned dv : 16;
	unsigned addr : 3;
	unsigned cmd : 3;
	unsigned resv : 2;
};

struct ad5663_priv_s {
	struct ad5663_chip_s *chip;
	struct ad5663_buffer_s *buffer;
};

#endif /*__AD5663_H_*/
