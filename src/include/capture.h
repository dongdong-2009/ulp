/*  capture.h
 * 	dusk@2010 initial version
 */
#ifndef __CAPTURE_H_
#define __CAPTURE_H_

#include "stm32f10x.h"

void capture_Init(void);
void capture_SetCounter(uint16_t count);
void capture_Start(void);
void capture_Stop(void);
uint16_t capture_GetCounter(void);

#endif /*__CAPTURE_H_*/
