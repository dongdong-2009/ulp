/*
 * 	king@2013 initial version
 */

#include "pid.h"

/*
 * 	y[n] = y[n-1] + A0 * x[n] + A1 * x[n-1] + A2 * x[n-2]
 * 	A0 = Kp + Ki + Kd
 * 	A1 = -Kp - (2 * Kd)
 * 	A2 = Kd
 */

void pid_q15_init(struct pid_q15_s *ppid)
{
	ppid->A[0] = ppid->kp + ppid->ki + ppid->kd;
	ppid->A[1] = -ppid->kp - (2 * ppid->kd);
	ppid->A[2] = ppid->kd;
	ppid->x[0] = 0;
	ppid->x[1] = 0;
	ppid->y = 0;
}

int pid_q15(struct pid_q15_s *ppid, int x)
{
	ppid->y = ppid->y + ppid->A[0] * x + ppid->A[1] * ppid->x[1] + ppid->A[2] * ppid->x[0];
	ppid->x[0] = ppid->x[1];
	ppid->x[1] = x;
	return ppid->y;
}
