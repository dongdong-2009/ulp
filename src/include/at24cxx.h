/*
 *	dusk@2010 initial version
 */

#ifndef __AT24CXXX_H
#define __AT24CXXX_H

#include "i2c.h"

#define AT24C02_ADDR 0xA0
#define AT24C02_PGSZ 0x08
#define AT24C02_ALEN 0x01

typedef struct {
	i2c_bus_t *bus;
	int page_size;	/*for continuous write*/
} at24cxx_t;

void at24cxx_Init(const at24cxx_t *chip);
int at24cxx_WriteByte(const at24cxx_t *chip, char chip_addr, int addr, int alen, const void *buffer);
int at24cxx_ReadByte(const at24cxx_t *chip, char chip_addr, int addr, int alen, void *buffer);
int at24cxx_WriteBuffer(const at24cxx_t *chip, char chip_addr, int addr, int alen, const void *buffer, int len);
int at24cxx_ReadBuffer(const at24cxx_t *chip, char chip_addr, int addr, int alen, void *buffer, int len);

#endif /* __AT24CXXX_H */
