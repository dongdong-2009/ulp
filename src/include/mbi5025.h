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
	int load_pin;	//use cs of spi as gpio control
	int oe_pin;	//use cs of spi as gpio control
} mbi5025_t;

void mbi5025_Init(mbi5025_t *chip);
void mbi5025_WriteByte(mbi5025_t *chip, unsigned char data);
void mbi5025_WriteBytes(mbi5025_t *chip, unsigned char * pdata, int len);
void mbi5025_EnableLoad(mbi5025_t *chip);
void mbi5025_DisableLoad(mbi5025_t *chip);
void mbi5025_EnableOE(mbi5025_t *chip);
void mbi5025_DisableOE(mbi5025_t *chip);

#endif /*__MBI5025_H_*/
