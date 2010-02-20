/* pwm.h
 * 	miaofng@2009 initial version
 */
#ifndef __DBG_H_
#define __DBG_H_

#include "stm32f10x.h"
#include "vsm.h"

/*pwm debug*/
#define pwm1_Set(val) do { TIM3->CCR1 = (((int)val * T >> 15) + T); } while(0)
#define pwm2_Set(val) do { TIM3->CCR2 = (((int)val * T >> 15) + T); } while(0)
#define pwm3_Set(val) do { TIM3->CCR3 = (((int)val * T >> 15) + T); } while(0)
#define pwm4_Set(val) do { TIM3->CCR4 = (((int)val * T >> 15) + T); } while(0)

/*gpio debug*/
/* pin map:
	GPIO_DEBUG1	PB11
	GPIO_DEBUG2	PB10
*/
#define BIT_DBG1 (1 << 11)
#define BIT_DBG2 (1 << 10)

#define _SDBG1 BIT_DBG1
#define _SDBG2 BIT_DBG2

#define _RDBG1 (BIT_DBG1 << 16)
#define _RDBG2 (BIT_DBG2 << 16)

#define dbg1_Set()	do { GPIOB->BSRR = _SDBG1; } while(0)
#define dbg1_Reset()	do { GPIOB->BSRR = _RDBG1; } while(0)
#define dbg1_Inv()	do { GPIOB->BSRR = (GPIOB->ODR & BIT_DBG1)?_RDBG1 : _SDBG1; } while(0)
#define dbg2_Set()	do { GPIOB->BSRR = _SDBG2; } while(0)
#define dbg2_Reset()	do { GPIOB->BSRR = _RDBG2; } while(0)
#define dbg2_Inv()	do { GPIOB->BSRR = (GPIOB->ODR & BIT_DBG2)?_RDBG2 : _SDBG2; } while(0)

void dbg_Init(void);

#endif /*__DBG_H_*/
