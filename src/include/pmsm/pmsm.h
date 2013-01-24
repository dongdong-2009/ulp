/*
 * 	king@2013 initial version
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

typedef struct {
	int pp;
	int Imax; //mA
	int Ud; //V
	int Umax; //V

	int crp;
} pmsm_t;

#endif /*__MOTOR_H_*/

