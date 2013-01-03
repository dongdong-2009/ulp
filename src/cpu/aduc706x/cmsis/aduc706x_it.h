/*
*  miaofng@2012-1-3 initial version
*/
#ifndef __ADUC706X_IT_H_
#define __ADUC706X_IT_H_

#include "config.h"

/*IRQ/FIQ MMR Bit Designations*/
#define FIQ_ALL (1 << 0) /*FIQ mmr only*/
#define IRQ_SWINT (1 << 1)
#define IRQ_TIM0 (1 << 3)
#define IRQ_TIM1 (1 << 4)
#define IRQ_TIM2 (1 << 5)
#define IRQ_TIM3 (1 << 6)
#define IRQ_ADC (1 << 10)
#define IRQ_UART (1 << 11)
#define IRQ_SPI (1 << 12)
#define IRQ_EINT0 (1 << 13)
#define IRQ_EINT1 (1 << 14)
#define IRQ_I2CM (1 << 15)
#define IRQ_I2CS (1 << 16
#define IRQ_PWM (1 << 17)
#define IRQ_EINT2 (1 << 18)
#define IRQ_EINT3 (1 << 19)

void NULL_ISR(void);
void TIM0_IRQHandler(void);
void TIM1_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void ADC_IRQHandler(void);
void UART_IRQHandler(void);
void SPI_IRQHandler(void);
void EINT0_IRQHandler(void);
void EINT1_IRQHandler(void);
void EINT2_IRQHandler(void);
void EINT3_IRQHandler(void);
void I2CM_IRQHandler(void);
void I2CS_IRQHandler(void);
void PWM_IRQHandler(void);

typedef void (*isr_t)(void);
extern const isr_t vectors_irq[];

#endif /*__ADUC706X_IT_H_*/
