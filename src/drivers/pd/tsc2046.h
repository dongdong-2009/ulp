/*
 * 	miaofng@2010 initial version
 */
#ifndef __TSC2046_H_
#define __TSC2046_H_

#include "spi.h"

#define START_BIT (1 << 7)
#define CHS_TMP0 (0x00 << 4 | 1 << 2)
#define CHS_POSY (0x01 << 4 | 1 << 2)
#define CHS_VBAT (0x02 << 4 | 1 << 2)
#define CHS_PRZ1 (0x03 << 4 | 1 << 2)
#define CHS_PRZ2 (0x04 << 4 | 1 << 2)
#define CHS_POSX (0x05 << 4 | 1 << 2)
#define CHS_AUXI (0x06 << 4 | 1 << 2)
#define CHS_TMP1 (0x07 << 4 | 1 << 2)
#define CHD_POSY (0x01 << 4 | 0 << 2)
#define CHD_PRZ1 (0x03 << 4 | 0 << 2)
#define CHD_PRZ2 (0x04 << 4 | 0 << 2)
#define CHD_POSX (0x05 << 4 | 0 << 2)
#define ADC_08BIT (1 << 3)
#define ADC_12BIT (0 << 3)
#define PD_ON (0x00)
#define PD_REF_OFF_ADC_ON (0x01)
#define PD_REF_ON_ADC_OFF (0x02)
#define PD_OFF (0x03)

typedef struct {
	const spi_bus_t *bus;
	int idx; //index of chip in the specified bus
} tsc2046_t;

int tsc2046_init(const tsc2046_t *chip);

#endif /*__TSC2046_H_*/
