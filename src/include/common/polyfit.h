/*
 * 	miaofng@2013-3-30 initial version
 */

#ifndef __POLYFIT_H_
#define __POLYFIT_H_

int gsl_fit_linear (const float *x, const float *y, const size_t n, float *c0, float *c1,
	/*float *cov_00, float *cov_01, float *cov_11, */float *sumsq );

#endif	/* __POLYFIT_H_ */
