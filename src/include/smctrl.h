/* smctrl.h
 * 	dusk@2010 initial version
 */
#ifndef __SMCTRL_H_
#define __SMCTRL_H_

#include "config.h"
#include "stm32f10x.h"

void smctrl_Init(void);
void smctrl_SetRPM(int rpm);
void smctrl_SetRotationDirection(bool dir);
void smctrl_Start(void);
void smctrl_Stop(void);
#endif /*__SMCTRL_H_*/
