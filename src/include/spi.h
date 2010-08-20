/*
*	miaofng@2010 initial version
*/
#ifndef __SPI_H_
#define __SPI_H_

#include <stddef.h>
#include "device.h"

typedef struct {
	unsigned cpol : 1; /*clock polarity, 0-> idle low level*/
	unsigned cpha : 1; /*clock phase, 0-> first edge active*/
	unsigned bits : 5; /*bits of a frame, 0~31*/
	unsigned bseq : 1; /*bit sequency, 0->lsb*/
} spi_cfg_t;

extern bus_t spi1;
extern bus_t spi2;
extern bus_t spi3;

#endif /*__SPI_H_*/

