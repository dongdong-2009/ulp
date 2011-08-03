/*
 * 	dusk@2011 initial version
 */
#ifndef __74HCT595_H_
#define __74HCT595_H_

#include "spi.h"

typedef struct {
	const spi_bus_t *bus;
	int idx; //index of chip in the specified bus
	int option;
} nxp_74hct595_t;

void nxp_74hct595_Init(nxp_74hct595_t *chip);
void nxp_74hct595_WriteByte(nxp_74hct595_t *chip, unsigned char data);

#endif /*__74HCT595_H_*/
