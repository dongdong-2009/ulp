/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_POWER_H_
#define __NEST_POWER_H_

#define RELAY_BAT_SET(ON) do { \
	if(ON) cncb_signal(BAT, LSD_OFF);  /*inverted on circuit*/ \
	else cncb_signal(BAT, LSD_ON); \
} while(0)

#define RELAY_IGN_SET(ON)  do { \
	if(ON) cncb_signal(IGN, LSD_OFF); /*inverted on circuit*/ \
	else cncb_signal(IGN, LSD_ON); \
} while(0)

#define RELAY_ETCBAT_SET(ON) do { \
	if(ON) cncb_signal(LSD, LSD_OFF); /*inverted on circuit*/ \
	else cncb_signal(LSD, LSD_ON); \
} while(0)

#define RELAY_LOCK_SET(ON)	do { \
	if(ON) cncb_signal(SIG4, SIG_HI); \
	else cncb_signal(SIG4, SIG_LO);  \
} while(0)

int nest_power_on(void);
int nest_power_off(void);
int nest_power_reboot(void);

#endif
