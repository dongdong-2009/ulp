#ifndef __SSS_CAPTURE_H_
#define __SSS_CAPTURE_H_

void capture_sss_Init(void);
void capture_Start(void);
void capture_Stop(void);
void capture_SetCounter(unsigned short count);
unsigned short capture_GetCounter(void);
void capture_ResetCounter(void);
void capture_SetAutoReload(unsigned short count);
unsigned short capture_GetAutoReload(void);
void capture_SetCounterModeUp(void);
void capture_SetCounterModeDown(void);
void TIM_DMAC_Init(void);
void DMAC_NVIC_Configuration(void);

#endif /*__CAPTURE_H_*/
