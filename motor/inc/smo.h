/*
 * 	miaofng@2009 initial version
 */
#ifndef __SMO_H_
#define __SMO_H_

#include "vector.h"

#ifndef CONFIG_PWM_FREQ
	#define CONFIG_PWM_FREQ (72000000/5000) /*unit: Hz*/
#endif

#ifndef CONFIG_MOTOR_START_TIME
	#define CONFIG_MOTOR_START_TIME (1000) /*unit: mS*/
#endif

#ifndef CONFIG_MOTOR_START_RPM
	#define CONFIG_MOTOR_START_RPM (500)
#endif

#define SMO_UPDATE_PERIOD	(100) /*unit: mS*/

typedef struct {
	short rs; /*stator resistance, unit mOhm*/
	short ld; /*d axis inductance*/
	short lq; /*q axis inductance*/
	short pn; /*nr of poles, such as 8*/
	short start_time; /*startup duration, unit: mS*/
	short start_speed; /*motor final startup speed in Hz*/
} motor_t;

extern motor_t *motor;

void smo_Init(void);
void smo_Update(void);
void smo_Reset(void);
void smo_isr(vector_t *pvs, vector_t *pis);
int smo_IsLocked(void); /*return yes(1)/no(0)*/
short smo_GetSpeed(void); /*unit: Hz*/
short smo_GetAngle(void);

#endif /*__SMO_H_*/

