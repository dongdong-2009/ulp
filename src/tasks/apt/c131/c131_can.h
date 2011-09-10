/*
 * David@2011,9 initial version
 */
#ifndef __C131_DIAG_H_
#define __C131_DIAG_H_

//Error define
#define ERROR_OK					0
#define ERROR_LOOP					-1
#define ERROR_SWITCH				-2
#define ERROR_LED					-3

void c131_diag_Init(void);
int c131_DiagSW(void);
int c131_DiagLED(void);
int c131_DiagLOOP(void);

#endif /*__C131_DIAG_H_*/
