/* board.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdlib.h>

pid_t *pid_Init(int kp, int ki, int kd)
{
	pid_t *pid = malloc(sizeof(pid_t));
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
	pid->ref = 0;
	pid->err_history = 0;
	return pid;
}

void pid_SetRef(pid_t *pid, int ref)
{
	pid->ref = ref;
}

int pid_Calcu(pid_t *pid, int in)
{
	int err, out;
	err = in - pid->ref;
	out = err * pid->kp;
	out += pid->err_history * pid->ki;
	pid->err_history = err;
	return out;
}

void pid_Close(pid_t *pid)
{
	free(pid);
}
