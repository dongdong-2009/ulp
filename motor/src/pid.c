/* board.c
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include <stdlib.h>
#include "pid.h"

pid_t *pid_Init(void)
{
	pid_t *pid = malloc(sizeof(pid_t));
	pid->kp = PID_KP_DEF;
	pid->ki = PID_KI_DEF;
	pid->kd = PID_KD_DEF;
	pid->ref = 0;
	pid->err_history = 0;
	return pid;
}

void pid_Config(pid_t *pid, short kp, short ki, short kd)
{
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
}

void pid_SetRef(pid_t *pid, short ref)
{
	pid->ref = ref;
}

short pid_Calcu(pid_t *pid, short in)
{
	short err, out;
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
