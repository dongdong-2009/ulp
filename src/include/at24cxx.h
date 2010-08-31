/*
 *	dusk@2010 initial version
 */

#ifndef __AT24CXXX_H
#define __AT24CXXX_H

#include "i2c.h"

typedef struct {
	i2c_bus_t *bus;
	int page_size;	/*for continuous write*/
} at24cxx_t;

void at24cxx_Init(at24cxx_t *chip);
int at24cxx_WriteByte(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer);
int at24cxx_ReadByte(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer);
int at24cxx_WriteBuffer(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer, int len);
int at24cxx_ReadBuffer(at24cxx_t *chip, unsigned char chip_addr, unsigned addr, int alen, unsigned char *buffer, int len);

#endif /* __AT24CXXX_H */
