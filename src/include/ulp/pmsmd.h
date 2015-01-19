/*
 * 	king@2013 initial version
 */

#ifndef __PMSMD_H
#define __PMSMD_H

typedef struct {
	void (*init)(int crp, int frq, int *tpwm, int *crp_elec);
	void (*encoder_set)(int counter);
	void (*encoder_ctl)(int enable);
	int (*encoder_get)(void);
	void (*svpwm_ctl)(int enable);
	void (*time_set)(int utime, int vtime, int wtime);
	void (*I_get)(int *Iu, int *Iv);
	void (*isr_ctl)(int enable);
} pmsmd_ops_t;

extern const pmsmd_ops_t pmsmd1_stm32;

#endif	/* __PMSMD_H */
