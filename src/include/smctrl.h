/* smctrl.h
 * 	dusk@2010 initial version
 */
#ifndef __SMCTRL_H_
#define __SMCTRL_H_

#include "stm32f10x.h"

#define CONFIG_STEPPERMOTOR_HALFSTEP
#define CONFIG_STEPPERMOTOR_FASTDECAYSTEP
#define CONFIG_STEPPERMOTOR_SPM	200
#define CONFIG_DRIVER_RPM_DDS_MCLK	25000000

void smctrl_Init(void);
void smctrl_SetRPM(int rpm);
void smctrl_SetRotationDirection(bool dir);
void smctrl_Start(void);
void smctrl_Stop(void);
#endif /*__SMCTRL_H_*/
