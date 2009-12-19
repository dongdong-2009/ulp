/* pwm.h
 * 	miaofng@2009 initial version
 */
#ifndef __PWM_H_
#define __PWM_H_

#include "stm32f10x.h"

void pwm_init(void);
void pwm_update(void);

/*freq: Hz, duty: 0~100*/
void pwm_config(int freq, int duty);
#define pwm_on() do{ TIM_Cmd(TIM3, ENABLE); } while(0)
#define pwm_off() do{ TIM_Cmd(TIM3, DISABLE); } while(0)

#endif /*__PWM_H_*/
