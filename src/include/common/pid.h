/*
 * 	king@2013 initial version
 */

#ifndef __PID_H
#define __PID_H

struct {
	int A[3];
	int kp;
	int ki;
	int kd;
	int x[2];
	int y;
} pid_q15_s;

void pid_q15_init(struct pid_q15_s *ppid);
int pid_q15(struct pid_q15_s *ppid, int x);

#endif	/* __PID_H */
