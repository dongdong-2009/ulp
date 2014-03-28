/*
*
* miaofng@2013-9-26
*
*/

#ifndef __YBSD_H__
#define __YBSD_H__

#include "aduc706x.h"

#define YBSD_ADC_PGA 512
#define YBSD_ADC_D2V(d) (d*1.2/(1<<23)/YBSD_ADC_PGA)
#define YBSD_DAC_V2D(v) (v/1.2*(1<<16))

int ybsd_vi_init(int adcflt);
static inline int ybsd_vi_read(int *p) { //ok return 1
	int status = ADCSTA;
	*p = ADC0DAT;
	return status;
}

int ybsd_vo_init(void);
int ybsd_set_vo(int data);

void ybsd_rb_init(void);
int ybsd_rb_get(void);

int ybsd_tx_enable(void);
int ybsd_rx_enable(void);

extern void ybs_isr(void);

#endif
