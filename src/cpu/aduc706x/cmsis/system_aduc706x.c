/*
	miaofng@2012-1-2 initial version
	The ADuC706x integrates a 32.768 kHz ¡À3% oscillator and a PLL.
	When powerup, the HCLK is equal to PLL clock divided by 8(CD=3,
	HCLK = 10.24MHz / 8 = 1.28MHz).
*/
#include "aduc706x.h"

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

void SysTick_Config(unsigned cd)
{
}

void SystemInit (void)
{
	SetSysClock(SystemFrequency);
}
