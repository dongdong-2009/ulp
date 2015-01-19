/*
 * 	king@2013 initial version
 */

#include "common/pid.h"

/*
 * 	A0 = Kp + Ki + Kd
 * 	A1 = -Kp - (2 * Kd)
 * 	A2 = Kd
 * 	A3 = Kp + Kd
 *
 * 	Èç¹ûmin < y[n-1] < max
 * 	y[n] = y[n-1] + A0 * x[n] + A1 * x[n-1] + A2 * x[n-2]
 *
 *
 *
 *
 */

void pid_q15_init(struct pid_q15_s *ppid)
{
	ppid->A[0] = ppid->kp + ppid->ki + ppid->kd;
	ppid->A[1] = -ppid->kp - (2 * ppid->kd);
	ppid->A[2] = ppid->kd;
	ppid->A[3] = ppid->kp + ppid->kd;
}

int pid_q15(struct pid_q15_s *ppid, int x)
{
	int min, max, y, temp;

	min = ppid->min;
	max = ppid->max;
	y = ppid->y;

	temp = y + ppid->A[1] * ppid->x[1] + ppid->A[2] * ppid->x[0];

	if((y > min && x < 0) || (y < max && x > 0)) {
		temp += ppid->A[0] * x;
	}
	else {
		temp += ppid->A[3] * x;
	}

	ppid->x[0] = ppid->x[1];
	ppid->x[1] = x;

	if(temp > max) {
		temp = max;
	}
	else if(temp < min) {
		temp = min;
	}
	ppid->y = temp;
	return temp;
}
