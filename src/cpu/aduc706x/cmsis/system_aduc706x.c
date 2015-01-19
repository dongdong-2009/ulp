/*
	miaofng@2012-1-2 initial version
	The ADuC706x integrates a 32.768 kHz ¡À3% oscillator and a PLL.
	When powerup, the HCLK is equal to PLL clock divided by 8(CD=3,
	HCLK(= core clock) = 10.24MHz(UCLK) / 8 = 1.28MHz).

*/
#include "aduc706x.h"

const unsigned SystemFrequency_UCLK = 10240000;
#if   defined CONFIG_SYSCLK_CD0
const unsigned SystemFrequency = (10240000/(1 << 0));
#elif defined CONFIG_SYSCLK_CD1
const unsigned SystemFrequency = (10240000/(1 << 1));
#elif defined CONFIG_SYSCLK_CD2
const unsigned SystemFrequency = (10240000/(1 << 2));
#elif defined CONFIG_SYSCLK_CD3
const unsigned SystemFrequency = (10240000/(1 << 3));
#elif defined CONFIG_SYSCLK_CD4
const unsigned SystemFrequency = (10240000/(1 << 4));
#elif defined CONFIG_SYSCLK_CD5
const unsigned SystemFrequency = (10240000/(1 << 5));
#elif defined CONFIG_SYSCLK_CD6
const unsigned SystemFrequency = (10240000/(1 << 6));
#elif defined CONFIG_SYSCLK_CD7
const unsigned SystemFrequency = (10240000/(1 << 7));
#else
const unsigned SystemFrequency = (10240000/(1 << 3));
#endif

void SetSysClock(unsigned hz)
{
	int cd;
	for(cd = 0; cd < 7; cd ++) {
		if(10240000 / (1 << cd) <= hz) {
			break;
		}
	}
	ClkSpd(cd);
}

/*
* ARM7TDMI do not like cm3 has systick timer,
* So TIM1 is used for it.
*/
void SysTick_Config(unsigned cd)
{
	TCfg(0, T_D1, T_HCLK, 1); //TIM0 in auto reload mode
	TMod(0, T_DOWN, T_BIN); //binary mode
	TGo(0, cd - 1, T_RUN);
	IRQEN |= IRQ_TIM0;
}

void SystemInit (void)
{
	SetSysClock(SystemFrequency);
#ifdef CONFIG_ADUC706X_VECTOR
	IRQBASE = ((unsigned) vectors_irq) >> 7;
	IRQCONN = 3;
	//IRQP0 = 0xFFFFFFFF; //lowest priority
	//IRQP1 = 0xFFFFFFFF;
	//IRQP2 = 0xFFFFFFFF;
#endif
	IRQCLR = 0xFFFF;
	FIQCLR = 0xFFFF;
	__enable_interrupt();
}
