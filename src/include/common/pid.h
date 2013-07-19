/*
 * 	king@2013 initial version
 */

/*
 * 	����������������PID�㷨��ֹ���ֱ���
 */

#ifndef __PID_H
#define __PID_H

struct pid_q15_s {
	int kp, ki, kd;
	int max, min;
	int A[4];
	int x[2];	//x[1]Ϊe[k-1], x[0]Ϊe[k-2]
	int y;
};

void pid_q15_init(struct pid_q15_s *ppid);
int pid_q15(struct pid_q15_s *ppid, int x);

#endif	/* __PID_H */
