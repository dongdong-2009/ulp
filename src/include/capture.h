/*  capture.h
 * 	dusk@2010 initial version
 */
#ifndef __CAPTURE_H_
#define __CAPTURE_H_

void capture_Init(void);
void capture_Start(void);
void capture_Stop(void);
void capture_SetCounter(unsigned short count);
unsigned short capture_GetCounter(void);
void capture_ResetCounter(void);
void capture_SetAutoReload(unsigned short count);
unsigned short capture_GetAutoReload(void);
void capture_SetCounterModeUp(void);
void capture_SetCounterModeDown(void);

#endif /*__CAPTURE_H_*/
