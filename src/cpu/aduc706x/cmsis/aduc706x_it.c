/* miaofng@2012-1-2
*/
#include "config.h"
#include "sys/task.h"
#include "aduc706x.h"
#include <stdlib.h>

#pragma data_alignment = 128
const isr_t vectors_irq[] = {
	NULL_ISR,                   /* 0  - Any FIQ active         */
	NULL_ISR,                   /* 1  - Software               */
	NULL_ISR,                   /* 2  - Unused                 */
	TIM0_IRQHandler,            /* 3  - Timer 0                */
	TIM1_IRQHandler,            /* 4  - Timer 1                */
	TIM2_IRQHandler,            /* 5  - Timer 2                */
	TIM3_IRQHandler,            /* 6  - Timer 3                */
	NULL_ISR,                   /* 7  - Unused                 */
	NULL_ISR,                   /* 8  - Unused                 */
	NULL_ISR,                   /* 9  - Unused                 */
	ADC_IRQHandler,             /* 10 - ADC Interrupt          */
	UART_IRQHandler,            /* 11 - UART interrupt         */
	SPI_IRQHandler,             /* 12 - SPI interrupt   */
	EINT0_IRQHandler,           /* 13 - External interrupt 0   */
	EINT1_IRQHandler,           /* 14 - External interrupt 1   */
	I2CM_IRQHandler,            /* 15 - I2C Master interrupt   */
	I2CS_IRQHandler,            /* 16 - I2C Slave interrupt    */
	PWM_IRQHandler,             /* 17 - PWM interrupt    */
	EINT2_IRQHandler,           /* 18 - External interrupt 2   */
	EINT3_IRQHandler,           /* 19 - External interrupt 3   */
	NULL_ISR,                   /* 20 - Unused                 */
	NULL_ISR,                   /* 21 - Unused                 */
	NULL_ISR,                   /* 22 - Unused                 */
	NULL_ISR,                   /* 23 - Unused                 */
	NULL_ISR,                   /* 24 - Unused                 */
	NULL_ISR,                   /* 25 - Unused                 */
	NULL_ISR,                   /* 26 - Unused                 */
	NULL_ISR,                   /* 27 - Unused                 */
	NULL_ISR,                   /* 28 - Unused                 */
	NULL_ISR,                   /* 29 - Unused                 */
	NULL_ISR,                   /* 30 - Unused                 */
	NULL_ISR,                   /* 31 - Unused                 */
};

/* these codes has been moved to cstartup.s
#ifndef CONFIG_ADUC706X_VECTOR
__irq __arm void IRQ_Handler(void)
{
	unsigned i, mask, status = IRQSTA;
	isr_t isr_handler;
	for(mask = 1, i = 0; i < 20; i ++, mask <<= 1) {
		if(status & mask) {
			isr_handler = vectors_irq[i];
			isr_handler();
		}
	}
}

__irq __arm void FIQ_Handler(void)
{
}
#endif
*/

__weak void NULL_ISR(void)
{
}

__weak void TIM0_IRQHandler(void)
{
	T0CLRI = 0;
	task_Isr();
}

__weak void TIM1_IRQHandler(void)
{
}

__weak void TIM2_IRQHandler(void)
{
}

__weak void TIM3_IRQHandler(void)
{
}

__weak void ADC_IRQHandler(void)
{
}

__weak void UART_IRQHandler(void)
{
}

__weak void SPI_IRQHandler(void)
{
}

__weak void EINT0_IRQHandler(void)
{
}

__weak void EINT1_IRQHandler(void)
{
}

__weak void EINT2_IRQHandler(void)
{
}

__weak void EINT3_IRQHandler(void)
{
}

__weak void I2CM_IRQHandler(void)
{
}

__weak void I2CS_IRQHandler(void)
{
}

__weak void PWM_IRQHandler(void)
{
}
