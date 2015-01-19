/*
 * 	king@2013 initial version
 */

/*
 * 	采用遇限削弱积分PID算法防止积分饱和
 */

#ifndef __PID_H
#define __PID_H

struct pid_q15_s {
	int kp, ki, kd;
	int max, min;
	int A[4];
	int x[2];	//x[1]为e[k-1], x[0]为e[k-2]
	int y;
};

void pid_q15_init(struct pid_q15_s *ppid);
int pid_q15(struct pid_q15_s *ppid, int x);

#endif	/* __PID_H */
