/*
 * 	miaofng@2010 initial version
 */
#ifndef __DAC1X8S_H_
#define __DAC1X8S_H_

#include "device.h"

typedef struct {
	spi_bus_t *bus;
	int idx; //index of chip in the specified bus
} dac1x8s_t;

void dac1x8s_Init(const dac1x8s_t *chip);
/*ch range: 0~7, val range: 0~4095*/
void dac1x8s_SetVolt(const dac1x8s_t *chip, int ch, int cnt);

#endif /*__DAC1X8S_H_*/
