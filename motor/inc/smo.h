/*
 * 	miaofng@2009 initial version
 */
#ifndef __SMO_H_
#define __SMO_H_

#include "vector.h"

#ifndef CONFIG_PWM_FREQ
	#define CONFIG_PWM_FREQ (72000000/5000) /*14.4Khz*/
#endif

typedef struct {
	short fs; /*sampling freq, unit: Hz*/
	int duration; /*startup duration, unit: mS*/
	short rpm; /*motor final startup rpm*/
} motor_config_t;

typedef struct {
	short rs; /*stator resistance, unit mOhm*/
	short ld; /*d axis inductance*/
	short lq; /*q axis inductance*/
	short pn; /*nr of poles, such as 8*/
	motor_config_t *config; /*reserved*/
} motor_t;

void smo_Init(void);
void smo_Reset(void);
void smo_SetMotor(motor_t *motor);
void smo_Update(vector_t *pvs, vector_t *pis);
int smo_IsReady(void); /*return yes(1)/no(0)*/
short smo_GetSpeed(void); /*unit: Hz*/
short smo_GetRPM(void); /*unit: RPM*/
short smo_GetAngle(void);

#endif /*__SMO_H_*/
