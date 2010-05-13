/* L6208.h
 * 	dusk@2010 initial version
 */
#ifndef __L6208_H_
#define __L6208_H_

#include "stm32f10x.h"

#define L6208_VREF	GPIO_Pin_0
#define L6208_RST	GPIO_Pin_11
#define L6208_ENA	GPIO_Pin_12
#define L6208_CTRL	GPIO_Pin_13
#define L6208_HALF	GPIO_Pin_14
#define L6208_DIR	GPIO_Pin_15

//this clock from dds
#define L6208_CLK	GPIO_Pin_8

/*define take pwm as l6208's reference voltage*/
#define L6208_VREF_USE_PWM
#ifdef L6208_VREF_USE_PWM
/*this unit is mv,and do not exceed 3300mv*/
#define L6208_VREF_VOLTAGE	1000
#endif


typedef enum{
	Clockwise = 0,
	CounterClockwise
}l6208_direction;

typedef enum{
	HalfMode = 0,
	FullMode
}l6208_stepmode;

typedef enum{
	DecaySlow = 0,
	DecayFast
}l6208_controlmode;

void l6208_Init(void);
void l6208_SetRotationDirection(int dir);
void l6208_SelectMode(l6208_stepmode stepmode);
void l6208_SetHomeState(FunctionalState state);
void l6208_SetControlMode(l6208_controlmode controlmode);
void l6208_StartCmd(FunctionalState state);
#endif /*__L6208_H_*/
