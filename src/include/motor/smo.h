/*
 * 	miaofng@2009 initial version
 */
#ifndef __SMO_H_
#define __SMO_H_

#include "vector.h"

#ifndef CONFIG_MOTOR_START_TIME
	#define CONFIG_MOTOR_START_TIME (1000) /*unit: mS*/
#endif

#ifndef CONFIG_MOTOR_START_RPM
	#define CONFIG_MOTOR_START_RPM (500)
#endif

#ifndef CONFIG_MOTOR_START_CURRENT
	#define CONFIG_MOTOR_START_CURRENT (10)
#endif

#define SMO_GAIN_SHIFT	11 /*n -> range: (+/-) 2^(-n) ~ 2^(15-n)*/
#define NOR_SMO_GAIN_VAL	(1.0/(1 << SMO_GAIN_SHIFT))
#define NOR_SMO_GAIN(g) (g  / NOR_SMO_GAIN_VAL)
#define _SMO_GAIN(g) (g * NOR_SMO_GAIN_VAL)

typedef struct {
	short rs;
	short ld;
	short lq;
	short pn; /*nr of poles, such as 8*/
	short start_time; /*startup duration, unit: mS*/
	short start_speed;
	short start_torque;
} motor_t;

extern motor_t *motor;

void smo_Init(void);
void smo_Update(void);
void smo_Start(void);
void smo_isr(vector_t *pvs, vector_t *pis);
int smo_IsLocked(void); /*return yes(1)/no(0)*/
short smo_GetSpeed(void); /*unit: Hz*/
short smo_GetAngle(void);

#endif /*__SMO_H_*/

