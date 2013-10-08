/*
*
* miaofng@2013-9-26
*
*/

#ifndef __YBSD_H__
#define __YBSD_H__

#include "aduc706x.h"

int ybsd_vi_init(void);
static inline int ybsd_vi_read(void) {
	return ADC0DAT;
}

/*vref: 0->used as gpio for RS485 DE/NRE usage, or 2500mv or 1200mv*/
int ybsd_vo_init(int vref_mv);
int ybsd_set_vo(int mv);
int ybsd_tx_enable(void);
int ybsd_rx_enable(void);

#endif
