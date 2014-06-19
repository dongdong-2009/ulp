/*
*
*  miaofng@2014-6-5   initial version
*
*/

#ifndef __BSP_H__
#define __BSP_H__

#include "config.h"

#define IRC_MASK_HV	(1<<1)
#define IRC_MASK_IS	(1<<5) //1, current source on, to measure resistor
#define IRC_MASK_LV	(1<<7) //1, low voltage source on, to power uut relay
#define IRC_MASK_EBUS	(1<<8) //1, bus switch = external
#define IRC_MASK_IBUS	(0<<8) //1, bus switch = internal
#define IRC_MASK_ELNE	(1<<9) //1, line switch = external
#define IRC_MASK_ILNE	(0<<8) //1, line switch = internal
#define IRC_MASK_EIS	(1<<10) //1, IS could be turn on
#define IRC_MASK_ELV	(1<<11) //1, LV could be turn on
#define IRC_MASK_ADD	(1<<11) //1, additional matrix switch operation is need

void board_init(void);
void board_reset(void);
void trig_set(int high);
int trig_get(void); //vmcomp pulsed
void le_set(int high);
int le_get(void);
void rly_set(int mode);

#endif
