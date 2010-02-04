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

