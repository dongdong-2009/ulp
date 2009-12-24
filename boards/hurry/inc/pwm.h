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

#define TIM3_COUNTER_VALUE	1000

void pwmdebug_init(void);
void pwmdebug_config(uint16_t ch,int freq, int duty);

#define pwmdebug_on() do{ TIM_Cmd(TIM3, ENABLE); } while(0)
#define pwmdebug_off() do{ TIM_Cmd(TIM3, DISABLE); } while(0)

#endif

#define PWM_U	TIM_Channel_1
#define PWM_V	TIM_Channel_2
#define PWM_W	TIM_Channel_3

#define TIM1_COUNTER_VALUE	1000
void pwm_init(void);
void pwm_update(void);

/*freq: Hz, duty: 0~100*/
void pwm_config(uint16_t ch,int freq, int duty, uint8_t repetition);
#define pwm_on() do{ TIM_Cmd(TIM1, ENABLE);TIM_CtrlPWMOutputs(TIM1, ENABLE);} while(0)
#define pwm_off() do{ TIM_Cmd(TIM1, DISABLE);TIM_CtrlPWMOutputs(TIM1, DISABLE); } while(0)

#endif /*__PWM_H_*/
