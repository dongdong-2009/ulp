/*
 * 	miaofng@2010 initial version
 *		ad9833, dds, max mclk=25MHz, 28bit phase resolution, 10bit DAC
 *		interface: spi, 40Mhz, 16bit, CPOL=0(sck low idle), CPHA=1(latch at 2nd edge of sck)
 */
#ifndef __AD9833_H_
#define __AD9833_H_

#include "device.h"

#define AD9833_OPT_OUT_SIN ((0 << 1) | (0 << 5)) /*sin waveform out enable*/
#define AD9833_OPT_OUT_TRI ((1 << 1) | (0 << 5)) /*triangle waveform out enable*/
#define AD9833_OPT_OUT_SQU ((0 << 1) | (1 << 5)) /*square waveform out enable, f/2*/
#define AD9833_OPT_DIV (1 << 3) /*Fout = f/2*/

typedef struct {
	dev_io_t io;
	int option;
	int priv;
} ad9833_t;

void ad9833_Init(ad9833_t *chip);
/*para 'freq' is the freq tuning word in case of 32bit phase resolution*/
void ad9833_SetFreq(ad9833_t *chip, unsigned fw);

#endif /*__AD9833_H_*/
