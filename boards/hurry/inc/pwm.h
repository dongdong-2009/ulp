/* pwm.h
 * 	miaofng@2009 initial version
 */
#ifndef __PWM_H_
#define __PWM_H_

#include "stm32f10x.h"

#define PWM_DEBUG

#ifdef PWM_DEBUG
#define PWMDEBUG_CH1	TIM_Channel_1
#define PWMDEBUG_CH2	TIM_Channel_2
#define PWMDEBUG_CH3	TIM_Channel_3
#define PWMDEBUG_CH4	TIM_Channel_4

void pwmdebug_init(void);
void pwmdebug_config(uint16_t ch,int freq, int duty);
#endif

void pwm_init(void);
void pwm_update(void);

/*freq: Hz, duty: 0~100*/
void pwm_config(int freq, int duty);
#define pwm_on() do{ TIM_Cmd(TIM3, ENABLE); } while(0)
#define pwm_off() do{ TIM_Cmd(TIM3, DISABLE); } while(0)

#endif /*__PWM_H_*/
