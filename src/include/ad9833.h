/*
 * 	miaofng@2010 initial version
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
} ad9833_t;

void ad9833_Init(ad9833_t *chip);
/*para 'freq' is the freq tuning word in case of 32bit phase resolution*/
void ad9833_SetFreq(ad9833_t *chip, unsigned fw);

/*note: ad9833_SetFreq can enable ad9833 automatically*/
void ad9833_Enable(const ad9833_t *chip);
void ad9833_Disable(const ad9833_t *chip);

#endif /*__AD9833_H_*/
