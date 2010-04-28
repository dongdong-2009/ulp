/*  capture.h
 * 	dusk@2010 initial version
 */
#ifndef __CAPTURE_H_
#define __CAPTURE_H_

#include "stm32f10x.h"

void capture_Init(void);
void capture_SetCounter(uint16_t count);
void capture_StartCmd(FunctionalState state);
void capture_isr(void);

#endif /*__CAPTURE_H_*/
