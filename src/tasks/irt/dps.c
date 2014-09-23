/*
*
*  miaofng@2014-6-4   initial version
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "dps.h"
#include "mxc.h"

void dps_init(void)
{
}

int dps_config(int key, float v)
{
	return 0;
}

static int cmd_power_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"POWER LV <value>	change lv voltage\n"
	};

	int key, ecode = -IRT_E_CMD_FORMAT;
	const char *name[] = {
		"LV", "IS", "HV",
	};
	const int key_list[] = {
		DPS_LV, DPS_IS, DPS_HV,
	};


	if((argc > 1) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
		return 0;
	}
	else if(argc == 3) {
		ecode = - IRT_E_CMD_PARA;
		float value = atof(argv[2]);
		for(int i = 0; i < sizeof(key_list) / sizeof(int); i ++) {
			if(!strcmp(argv[1], name[i])) {
				key = key_list[i];
				ecode = dps_config(key, value);
				break;
			}
		}
	}

	irc_error_print(ecode);
	return 0;
}
const cmd_t cmd_power = {"POWER", cmd_power_func, "power board ctrl"};
DECLARE_SHELL_CMD(cmd_power)

