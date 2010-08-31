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
	SPI_CS_PA4, /*SPI1_NSS*/
	SPI_1_NSS = SPI_CS_PA4,
	SPI_CS_PB1,
	SPI_CS_PB10,
	SPI_CS_PB12, /*SPI2_NSS*/
	SPI_2_NSS = SPI_CS_PB12,
	SPI_CS_PC4,
	SPI_CS_PC5,
	SPI_CS_PF11,
};

//private
int spi_cs_init(void);
int spi_cs_set(int pin, int level);

typedef struct {
	unsigned cpol : 1; /*clock polarity, 0-> idle low level*/
	unsigned cpha : 1; /*clock phase, 0-> first edge active*/
	unsigned bits : 5; /*bits of a frame, 0~31*/
	unsigned bseq : 1; /*bit sequency, 0->lsb*/
	unsigned csel : 1; /*csel on/off, 1 -> driver will control cs signal by itself */
} spi_cfg_t;

#define SPI_CFG_DEF { \
	.csel = 0, \
}

typedef struct {
	int (*init)(const spi_cfg_t *cfg);
	int (*wreg)(int addr, int val);
	int (*rreg)(int addr);
	int (*csel)(int idx, int level);
	
	/*reserved*/
	int (*wbuf)(char *buf, int n);
	int (*rbuf)(char *buf, int n);
} spi_bus_t;

extern spi_bus_t spi1;
extern spi_bus_t spi2;
extern spi_bus_t spi3;

#endif /*__SPI_H_*/

