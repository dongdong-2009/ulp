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
void pwmdebug_Init(void);
void pwmdebug_Config(unsigned short ch, int duty);
#define pwmdebug_On() do{ TIM_Cmd(TIM3, ENABLE); } while(0)
#define pwmdebug_Off() do{ TIM_Cmd(TIM3, DISABLE); } while(0)
#endif

/* PWM Peripheral Input clock */
#define CKTIM	((u32)72000000uL)
/* ADC IRQ-HANDLER frequency, related to PWM  */
#define REP_RATE (1)  // (N.b): Internal current loop is performed every 
                      //             (REP_RATE + 1)/(2*PWM_FREQ) seconds.
                      // REP_RATE has to be an odd number in case of three-shunt
                      // current reading; this limitation doesn't apply to ICS

//Not to be modified
//#define SAMPLING_FREQ   ((u16)PWM_FREQ/((REP_RATE+1)/2))   // Resolution: 1Hz

/* Power devices switching frequency */
#define PWM_FREQ ((u16) 14400) // in Hz  (N.b.: pattern type is center aligned)
/* Deadtime Value */
#define DEADTIME_NS	((u16) 800)  //in nsec; range is [0...3500] 
#define DEADTIME  (u16)((unsigned long long)CKTIM/2*(unsigned long long)DEADTIME_NS/1000000000uL) 

/* PWM Frequency */
#define PWM_PRSC ((u8)0)         
#define PWM_PERIOD ((u16) (CKTIM / (u32)(2 * PWM_FREQ *(PWM_PRSC+1)))) 
    
#define PWM_U	TIM_Channel_1
#define PWM_V	TIM_Channel_2
#define PWM_W	TIM_Channel_3

void pwm_Init(void);
void pwm_Update(void);
void pwm_Config(unsigned short ch, int duty, unsigned char repetition);
#define pwm_On() do{ TIM_Cmd(TIM1, ENABLE);TIM_CtrlPWMOutputs(TIM1, ENABLE);} while(0)
#define pwm_Off() do{ TIM_Cmd(TIM1, DISABLE);TIM_CtrlPWMOutputs(TIM1, DISABLE); } while(0)

#endif /*__PWM_H_*/
