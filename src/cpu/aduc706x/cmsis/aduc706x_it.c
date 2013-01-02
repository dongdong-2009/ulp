/* miaofng@2012-1-2
*/
#include "config.h"
#include "sys/task.h"
#include "aduc706x.h"

__irq __arm void IRQ_Handler(void)
{
	unsigned status = IRQSTA;
	if(status & IRQ_TIM0) {
		T0CLRI = 0;
		task_Isr();
	}
}

__irq __arm void FIQ_Handler(void)
{
}
