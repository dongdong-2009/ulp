/* miaofng@2012-1-2 initial version
*/
#ifndef __SYSTEM_ADUC706X_H_
#define __SYSTEM_ADUC706X_H_

extern const unsigned SystemFrequency;
extern const unsigned SystemFrequency_UCLK;

void SetSysClock(unsigned hz);
void SysTick_Config(unsigned cd);

#endif /*__SYSTEM_ADUC706X_H_*/
