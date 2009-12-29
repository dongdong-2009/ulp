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
void pwmdebug_Init(void);
void pwmdebug_Config(uint16_t ch,int freq, int duty);
#define pwmdebug_On() do{ TIM_Cmd(TIM3, ENABLE); } while(0)
#define pwmdebug_Off() do{ TIM_Cmd(TIM3, DISABLE); } while(0)
#endif
/****	ADC IRQ-HANDLER frequency, related to PWM  ****/
#define REP_RATE (1)  // (N.b): Internal current loop is performed every 
                      //             (REP_RATE + 1)/(2*PWM_FREQ) seconds.
                      // REP_RATE has to be an odd number in case of three-shunt
                      // current reading; this limitation doesn't apply to ICS

//Not to be modified
//#define SAMPLING_FREQ   ((u16)PWM_FREQ/((REP_RATE+1)/2))   // Resolution: 1Hz

////////////////////////////// Deadtime Value /////////////////////////////////
//#define DEADTIME  (u16)((unsigned long long)CKTIM/2*(unsigned long long)DEADTIME_NS/1000000000uL) 

#define PWM_U	TIM_Channel_1
#define PWM_V	TIM_Channel_2
#define PWM_W	TIM_Channel_3

//#define TIM1_COUNTER_VALUE	1000
#define TIM1_COUNTER_VALUE	2500
#define PWM_PRSC ((u8)0)

void pwm_Init(void);
void pwm_Update(void);

/*freq: Hz, duty: 0~100*/
void pwm_Config(uint16_t ch,int freq, int duty, uint8_t repetition);
#define pwm_On() do{ TIM_Cmd(TIM1, ENABLE);TIM_CtrlPWMOutputs(TIM1, ENABLE);} while(0)
#define pwm_Off() do{ TIM_Cmd(TIM1, DISABLE);TIM_CtrlPWMOutputs(TIM1, DISABLE); } while(0)

#endif /*__PWM_H_*/
