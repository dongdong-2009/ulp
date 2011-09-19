/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_POWER_H_
#define __NEST_POWER_H_

#include "cncb.h"

#if(!defined(CONFIG_NEST_MT60_OLD) && !defined(CONFIG_NEST_MT80_OLD))
static inline void RELAY_BAT_SET(int ON)
{
	if(ON) cncb_signal(BAT, LSD_OFF);  /*inverted on circuit*/
	else cncb_signal(BAT, LSD_ON);
}
static inline void RELAY_IGN_SET(int ON) 
{
	if(ON) cncb_signal(IGN, LSD_OFF); /*inverted on circuit*/
	else cncb_signal(IGN, LSD_ON);
}
static inline void RELAY_ETCBAT_SET(int ON)
{
	if(ON) cncb_signal(LSD, LSD_OFF); /*inverted on circuit*/
	else cncb_signal(LSD, LSD_ON);
}
static inline void RELAY_LOCK_SET(int ON)
{
	if(ON) cncb_signal(SIG4, SIG_HI);
	else cncb_signal(SIG4, SIG_LO);
}
#else /*CONFIG_NEST_MT60_OLD*/
static inline void RELAY_BAT_SET(int ON)
{
	if(ON) cncb_signal(BAT, LSD_ON);
	else cncb_signal(BAT, LSD_OFF);
}
static inline void RELAY_IGN_SET(int ON)
{
	if(ON) cncb_signal(IGN, LSD_ON); \
	else cncb_signal(IGN, LSD_OFF); \
}
static inline void RELAY_ETCBAT_SET(int ON)
{
	if(ON) cncb_signal(LSD, LSD_ON);
	else cncb_signal(LSD, LSD_OFF);
}
static inline void RELAY_LOCK_SET(int ON)
{
#ifndef CONFIG_NEST_MT80_OLD
	if(ON) cncb_signal(SIG3, SIG_HI);
	else cncb_signal(SIG3, SIG_LO);
#endif
}
#endif /*CONFIG_NEST_MT60_OLD*/

int nest_power_on(void);
int nest_power_off(void);
int nest_power_reboot(void);

#endif
