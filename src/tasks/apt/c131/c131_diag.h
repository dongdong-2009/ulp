/*
 * David@2011,9 initial version
 */
#ifndef __C131_DIAG_H_
#define __C131_DIAG_H_

//diagnosis debug define
#define DIAG_DEBUG		1

//Error define
#define ERROR_OK					0
#define ERROR_LOOP					-1
#define ERROR_SWITCH				-2
#define ERROR_LED					-3

//define diagnosis limitation
//0.95V = 0x0614
#define LOOP_HIGH_LIMIT		0x0680
#define LOOP_LOW_LIMIT		0x0560

#define SW1_6MA_HIGH_LIMIT	0x03ff
#define SW1_6MA_LOW_LIMIT	0x03bb
#define SW1_14MA_HIGH_LIMIT	0x0980
#define SW1_14MA_LOW_LIMIT	0x0880

//820 -> 0x79e
//2k82 -> 0xc1e
#define SW2_820_HIGH_LIMIT	0x07ff
#define SW2_820_LOW_LIMIT	0x0700
#define SW2_2K82_HIGH_LIMIT	0x0c80
#define SW2_2K82_LOW_LIMIT	0x0b80

//220 -> 0x7bc
//900 -> 0xcb2
#define SW5_900_HIGH_LIMIT	0x0cff
#define SW5_900_LOW_LIMIT	0x0c80
#define SW5_220_HIGH_LIMIT	0x07ff
#define SW5_220_LOW_LIMIT	0x0780

//220 -> 0x7bc
//900 -> 0xcb2
#define LED_HIGH_LIMIT	0x0fff
#define LED_LOW_LIMIT	0x0000

void c131_diag_Init(void);
int c131_DiagSW(void);
int c131_DiagLED(void);
int c131_DiagLOOP(void);

#endif /*__C131_DIAG_H_*/
