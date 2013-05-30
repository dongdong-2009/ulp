/*
 * 	king@2013 initial version
 */

#ifndef __FOC_H
#define __FOC_H
#include "pmsm/pmsm.h"
#include "ulp/pmsmd.h"
#include "common/pid.h"

typedef struct {
	int status;
	int frq;
	int crp;
	int pid_flag; //pid has been completed
	int execute_update; //SVMWM module has executed the changes
	pmsmd_ops_t pmsmd;
	pmsm_t pmsm;

	struct {
		int theta_per_counter_q10;

		int theta_before;
		int theta_now;
		int theta_expect;
		int theta_next;
		int theta_diff;

		int speed_now;
		int speed_expect;
		int speed_calc_cnt;
		int speed_calc_cnt_max;
		int speed_theta_diff;

		int acclr_now;
		int acclr_expect;
	} mech;

	struct {
		int theta;
		int iu;
		int iv;
		int id;
		int iq;
		int ualpha;
		int ubeta;
		int tpwm;
		int tpwm_multi_sqrt3;
	} elec;

	struct pid_q15_s id_pid;
	struct pid_q15_s iq_pid;
	struct pid_q15_s speed_pid;
	struct pid_q15_s theta_pid;
	struct pid_q15_s acclr_pid;
} foc_t;

int foc_new(void);
void foc_cfg(int handle, ...);
void foc_init(int handle);
void foc_update(int handle);
void foc_speed_set(int handle, int rpm);
void foc_theta_set(int handle, int theta);
void foc_acclr_set(int handle, int acclr); //set acceleration
void foc_isr(int handle);

#endif	/* __FOC_H */
