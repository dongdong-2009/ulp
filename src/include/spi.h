/*
*	miaofng@2010 initial version
*/
#ifndef __SPI_H_
#define __SPI_H_

#include <stddef.h>
#include "device.h"
#include "config.h"

enum {
	SPI_CS_DUMMY = 0,
	SPI_CS_PB1,
	SPI_CS_PB10,
	SPI_CS_PC4,
	SPI_CS_PC5,
	SPI_CS_PF11,
};

//private
void spi_cs_init(void);
void spi_cs_set(int pin, int level);

typedef struct {
	unsigned cpol : 1; /*clock polarity, 0-> idle low level*/
	unsigned cpha : 1; /*clock phase, 0-> first edge active*/
	unsigned bits : 5; /*bits of a frame, 0~31*/
	unsigned bseq : 1; /*bit sequency, 0->lsb*/
} spi_cfg_t;

typedef struct {
	int (*init)(const spi_cfg_t *cfg);
	int (*wreg)(int addr, int val);
	int (*rreg)(int addr);
	
	/*reserved*/
	int (*wbuf)(char *buf, int n);
	int (*rbuf)(char *buf, int n);
} spi_bus_t;

extern spi_bus_t spi1;
extern spi_bus_t spi2;
extern spi_bus_t spi3;

#endif /*__SPI_H_*/

