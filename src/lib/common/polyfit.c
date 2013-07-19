/* fit/linear.c
 *
 * Copyright (C) 2000, 2007 Brian Gough
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>

/* Fit the data (x_i, y_i) to the linear relationship

   Y = c0 + c1 x

   returning,

   c0, c1  --  coefficients
   cov00, cov01, cov11  --  variance-covariance matrix of c0 and c1,
   sumsq   --   sum of squares of residuals

   This fit can be used in the case where the errors for the data are
   uknown, but assumed equal for all points. The resulting
   variance-covariance matrix estimates the error in the coefficients
   from the observed variance of the points around the best fit line.
*/

int gsl_fit_linear (const float *x, const float *y, const size_t n, float *c0, float *c1,
	/*float *cov_00, float *cov_01, float *cov_11, */float *sumsq )
{
	float m_x = 0, m_y = 0, m_dx2 = 0, m_dxdy = 0;
	size_t i;

	for (i = 0; i < n; i++) {
		m_x += (x[i] - m_x) / (i + 1.0);
		m_y += (y[i] - m_y) / (i + 1.0);
	}

	for (i = 0; i < n; i++) {
		const float dx = x[i] - m_x;
		const float dy = y[i] - m_y;

		m_dx2 += (dx * dx - m_dx2) / (i + 1.0);
		m_dxdy += (dx * dy - m_dxdy) / (i + 1.0);
	}

	/* In terms of y = a + b x */
	float s2 = 0, d2 = 0;
	float b = m_dxdy / m_dx2;
	float a = m_y - m_x * b;

	*c0 = a;
	*c1 = b;

	/* Compute chi^2 = \sum (y_i - (a + b * x_i))^2 */
	for (i = 0; i < n; i++) {
		const float dx = x[i] - m_x;
		const float dy = y[i] - m_y;
		const float d = dy - b * dx;
		d2 += d * d;
	}

	s2 = d2 / (n - 2.0);		/* chisq per degree of freedom */
#if 0
	*cov_00 = s2 * (1.0 / n) * (1 + m_x * m_x / m_dx2);
	*cov_11 = s2 * 1.0 / (n * m_dx2);

	*cov_01 = s2 * (-m_x) / (n * m_dx2);
#endif

	*sumsq = d2;
	return 0;
}

#if 0
const float x[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const float y[] = {0, 195, 141, 241, 495, 500, 504, 758, 858, 904, 1000};

int main(int argc, char *argv[])
{
	float c0 = 0, c1 = 0;
	float cov_00, cov_01, cov_11, sumsq;
	gsl_fit_linear(x, y, 11, &c0, &c1, &cov_00, &cov_01, &cov_11, &sumsq);
	printf("c0 = %f, c1 = %f\n", c0, c1);
	printf("cov_00 = %f, cov_01 = %f, cov_11 = %f, sumsq = %f\n", cov_00, cov_01, cov_11, sumsq);
	return 0;
}
#endif
