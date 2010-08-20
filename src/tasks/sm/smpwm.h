/*
 * 	dusk@2010 initial version
 */
#ifndef __SMPWM_H_
#define __SMPWM_H_

#define SM_PWM_PRIOD 999

void smpwm_Init(void);
void smpwm_SetFrequency(int fre);
void smpwm_SetDuty(int duty);

#endif /*__AD9833_H_*/
