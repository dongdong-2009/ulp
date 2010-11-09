/*
 *  Copyright (C) 2010 miaofng
 *
 */

#include "config.h"

#define PDL_DEF { \
	.a = {1, 0, 0, 0, 1, 0, 1}, \
}

typedef struct {
	// Linear scaling and offset parameters for x,y (can include rotation)
	int a[7];
} pdl_t;

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
} pdl_cal_t;

/* this routine is used to calibrate the touch screen sample point
with linear algo specified by para of pdl, return 0 indicate success*/
int pdl_process(const pdl_t *pdl, struct pd_sample *sp);

/* this routine is used to calibrate para of the  linear algo, at least 5 group data should be
provided, return 0 indicate sucess*/
int pdl_cal(const pdl_cal_t *cal, pdl_t *pdl);
