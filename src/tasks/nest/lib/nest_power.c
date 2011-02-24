/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "cncb.h"
#include "nest_power.h"
#include "nest_core.h"
#include "nest_light.h"
#include "nest_message.h"

int nest_power_on(void)
{
	nest_message("Nest Power On\n");
	// flash led for visual verify.
	nest_light(ALL_ON);
	nest_mdelay(100);
	nest_light(ALL_OFF);

	//enable solenoid
	RELAY_LOCK_SET(1);

	// Set nest's LED indicators and apply power to target.
	nest_light(RUNNING_ON);
	RELAY_BAT_SET(1);
	RELAY_ETCBAT_SET(1);
	nest_mdelay(100);
	RELAY_IGN_SET(1);
	nest_mdelay(500);
	return 0;
}

int nest_power_off(void)
{
	nest_message("Nest PowerOff\n");
	nest_light(ALL_OFF);          // Turn all indicators off.

	//release solenoid
	RELAY_LOCK_SET(0);

	//BAT&IGN&ETCBAT
	RELAY_IGN_SET(0);
	nest_mdelay(10);
	RELAY_BAT_SET(0);
	RELAY_ETCBAT_SET(0);
	nest_mdelay(10);
	return 0;
}

int nest_power_reboot(void)
{
	nest_message("Nest Reboot\n");
	/*Do not switch on/off BAT relay!!! We need to lock the DUT on the fixture by that signal*/
	nest_light(RUNNING_OFF);
	RELAY_IGN_SET(0);
	nest_mdelay(10);
	RELAY_ETCBAT_SET(0);
	RELAY_BAT_SET(0);
	nest_mdelay(1000);

	//DelayS(1); //wait for capacitor discharge
	nest_light(RUNNING_ON);
	RELAY_BAT_SET(1);
	RELAY_ETCBAT_SET(1);
	nest_mdelay(100);
	RELAY_IGN_SET(1);
	nest_mdelay(500);
	return 0;
}
