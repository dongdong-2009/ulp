/*
 * David@2011,9 initial version
 */
#ifndef __C131_DIAG_H_
#define __C131_DIAG_H_

//diagnosis debug define
#define DIAG_DEBUG		1
#define DIAG_LCD		1

//Error define
#define ERROR_OK					0
#define ERROR_LOOP					-1
#define ERROR_SWITCH				-2
#define ERROR_LED					-3

//define diagnosis limitation
//0.95V = 0x0614
#define LOOP_HIGH_LIMIT		0x0c00
#define LOOP_LOW_LIMIT		0x0400

//6MA -> 0x3d7
//14MA -> 0x8f5
#define SW1_6MA_HIGH_LIMIT	0x0580
#define SW1_6MA_LOW_LIMIT	0x0320
#define SW1_14MA_HIGH_LIMIT	0x0a80
#define SW1_14MA_LOW_LIMIT	0x0800

//820 -> 0x6ee
//2k82 -> 0xf44
#define SW2_820_HIGH_LIMIT	0x0800
#define SW2_820_LOW_LIMIT	0x0600
#define SW2_2K82_HIGH_LIMIT	0x0fff
#define SW2_2K82_LOW_LIMIT	0x0e00

//220 -> 0x5dd
//900 -> 0xe92
#define SW5_900_HIGH_LIMIT	0x0fff
#define SW5_900_LOW_LIMIT	0x0d00
#define SW5_220_HIGH_LIMIT	0x0700
#define SW5_220_LOW_LIMIT	0x0500

//220 -> 0x7bc
//900 -> 0xcb2
#define LED_HIGH_LIMIT	0x0fff
#define LED_LOW_LIMIT	0x0000

void c131_diag_Init(void);
int c131_DiagSW(void);
int c131_DiagLED(void);
int c131_DiagLOOP(void);
int c131_GetDiagLoop(int * *pdata, int * *pflag, int * pdata_len);
int c131_GetDiagSwitch(int * *pdata, int * *pflag, int * pdata_len);

#endif /*__C131_DIAG_H_*/
