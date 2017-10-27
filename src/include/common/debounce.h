/*
 *	miaofng@2012 initial version
 *
 */
#ifndef __COMMON_DEBOUNCE_H_
#define __COMMON_DEBOUNCE_H_

#include "ulp_time.h"

struct debounce_s {
	unsigned on : 1; //level after debounce
	unsigned off : 1; // =not(on) always
	unsigned cnt : 15;
	unsigned threshold : 15;
	time_t timer; //last updated time
};

void debounce_init(struct debounce_s *signal, unsigned threshold, unsigned init_lvl);
int debounce(struct debounce_s *signal, unsigned lvl);

void debounce_t_init(struct debounce_s *signal, unsigned threshold_ms, unsigned init_lvl);

#endif /*__COMMON_DEBOUNCE_H_*/
