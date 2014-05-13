/*
*	miaofng@2010 initial version
*/
#ifndef __SPI_H_
#define __SPI_H_

#include <stddef.h>
#include "config.h"

enum {
	SPI_CS_DUMMY = 0,

	SPI_CS_PA2,
	SPI_CS_PA3,
	SPI_CS_PA4, /*SPI1_NSS*/
	SPI_1_NSS = SPI_CS_PA4,
	SPI_CS_PA12,

	SPI_CS_PB0,
	SPI_CS_PB1,
	SPI_CS_PB6,
	SPI_CS_PB7,
	SPI_CS_PB10,
	SPI_CS_PB12, /*SPI2_NSS*/
	SPI_2_NSS = SPI_CS_PB12,

	SPI_CS_PC3,
	SPI_CS_PC4,
	SPI_CS_PC5,
	SPI_CS_PC6,
	SPI_CS_PC7,
	SPI_CS_PC8,

	SPI_CS_PD8,
	SPI_CS_PD9,
	SPI_CS_PD12,

	SPI_CS_PF11,

#if CONFIG_CPU_ADUC706X == 1
	SPI_CS_P00,
#endif

	SPI_CS_NONE,
};

//private
int spi_cs_init(void);
int spi_cs_set(int pin, int level);

typedef struct {
	unsigned cpol : 1; /*clock polarity, 0-> idle low level*/
	unsigned cpha : 1; /*clock phase, 0-> first edge active*/
	unsigned bits : 6; /*bits of a frame, 1~32*/
	unsigned bseq : 1; /*bit sequency, 0->lsb*/
	unsigned csel : 1; /*customized select control through by csel() method */
	unsigned freq;
} spi_cfg_t;

#define SPI_CFG_DEF { \
	.csel = 0, \
	.freq = 0, /*default spi bus freq*/ \
}

typedef struct {
	int (*init)(const spi_cfg_t *cfg);
	int (*wreg)(int addr, int val);
	int (*rreg)(int addr);
	int (*csel)(int idx, int level);

	int (*wbuf)(const char *wbuf, char *rbuf, int n);
	int (*poll)(void); //0 indicates tranfser finished
} spi_bus_t;

extern const spi_bus_t spi0;
extern const spi_bus_t spi1;
extern const spi_bus_t spi2;
extern const spi_bus_t spi3;

#endif /*__SPI_H_*/

