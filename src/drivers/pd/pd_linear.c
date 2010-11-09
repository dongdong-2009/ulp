/*
 *  tslib/plugins/linear.c
 *
 *  Copyright (C) 2001 Russell King.
 *  Copyright (C) 2005 Alberto Mardegan <mardy@sourceforge.net>
 *  Copyright (C) 2010 miaofng
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Linearly scale touchscreen values
 */

#include "config.h"
#include "pd.h"
#include "pd_linear.h"

//#define _DEBUG

int pdl_process(const pdl_t *pdl, struct pd_sample *sp)
{
	int x, y, z;

	x = sp -> x;
	y = sp -> y;
	z = sp -> z;
	sp -> x = ( pdl -> a[1] * x + pdl -> a[2] * y + pdl -> a[0]) / pdl -> a[6];
	sp -> y = ( pdl -> a[4] * x + pdl -> a[5] * y + pdl -> a[3]) / pdl -> a[6];

#ifdef _DEBUG
	printf("pdl: ( %d %d %d ) -> ( %d %d %d )\n", x, y, z, sp -> x, sp -> y, sp -> z);
#endif /*_DEBUG*/

	return 0;
}

int pdl_cal(const pdl_cal_t *cal, pdl_t *pdl)
{
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

	// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

	// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
#ifdef _DEBUG
		printf("pdl: determinant is too small -- %f\n",det);
#endif /*_DEBUG*/
		return -1;
	}

	// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

	// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

	// Now multiply out to get the calibration for framebuffer x coord
	pdl->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	pdl->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	pdl->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

#ifdef _DEBUG
	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));
#endif /*_DEBUG*/

	// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

	// Now multiply out to get the calibration for framebuffer y coord
	pdl->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	pdl->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	pdl->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

#ifdef _DEBUG
	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));
#endif /*_DEBUG*/

	// If we got here, we're OK, so assign scaling to a[6] and return
	pdl->a[6] = (int)scaling;
	return 0;
}
