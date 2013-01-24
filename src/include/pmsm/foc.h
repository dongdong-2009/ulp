/*
 * 	king@2013 initial version
 */

#ifndef __FOC_H
#define __FOC_H

enum {
	FRQ,
	PMSM,
	BOARD,
	ID_PID,
	IQ_PID,
	SPEED_PID,
	THETA_PID,
	ACCLR_PID,
};

int foc_new(void);
void foc_cfg(int handle, ...);
void foc_init(int handle);
void foc_update(int handle);
void foc_speed_set(int handle, int rpm);
void foc_theta_set(int handle, int theta);
void foc_acclr_set(int handle, int acclr); //set acceleration
void foc_isr(int handle);

#endif	/* __FOC_H */
