/*
 * 	dusk@2011 initial version
 */
#ifndef __MBI5025_H_
#define __MBI5025_H_

#include "spi.h"

typedef struct {
	const spi_bus_t *bus;
	int idx; //index of chip in the specified bus
	int option;
} mbi5025_t;

void mbi5025_Init(mbi5025_t *chip);
void mbi5025_WriteByte(mbi5025_t *chip, unsigned char data);

#endif /*__MBI5025_H_*/
