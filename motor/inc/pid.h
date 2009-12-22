/* board.h
 * 	miaofng@2009 initial version
 */
#ifndef __PID_H_
#define __PID_H_

typedef struct {
	int kp;
	int ki;
	int kd; /*not used yet*/
	
	/*private, pls don't touch me;)*/
	int ref;
	int err_history;
} pid_t;

pid_t *pid_Init(int kp, int ki, int kd);
void pid_SetRef(pid_t *pid, int ref);
int pid_Calcu(pid_t *pid, int in);
void pid_Close(pid_t *pid);
#endif /*__PID_H_*/
