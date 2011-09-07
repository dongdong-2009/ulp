/*
 * David@2011 init version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "shell/cmd.h"
#include "spi.h"
#include "c131_relay.h"
#include "c131_misc.h"
#include "c131_diag.h"
#include "time.h"
#include "mbi5025.h"

static c131_load_t newload;

static int cmd_c131_func(int argc, char *argv[])
{
	int temp;
	int i;

	const char * usage = { \
		" usage:\n" \
		" c131 ess on/off nESS, config nESS on/off \n" \
		" c131 loop on/off nLOOP, config nLOOP on/off \n" \
		" c131 led on/off nLED, config nLED on/off \n" \
		" c131 switch on/off nSWITCH, config nSWITCH on/off \n" \
		" c131 power on/off sdm/led,  config sdm/led power on/off \n" \
		" c131 add load_name b0 b1 ... b7,  add new config \n" \
	};

	if (Get_C131Mode() == C131_MODE_NORMAL){
		printf("In normal mode\n");
		return 0;
	}

	if (argc > 1) {
		//for ess simulator
		if(strcmp(argv[1], "ess") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on"))
				ess_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off"))
				ess_SetRelayStatus(0x01 << temp, RELAY_OFF);
			return 0;
		}

		//for loop simulator
		if(strcmp(argv[1], "loop") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on"))
				loop_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off"))
				loop_SetRelayStatus(0x01 << temp, RELAY_OFF);
			return 0;
		}

		//for led simulator
		if(strcmp(argv[1], "led") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on"))
				led_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off"))
				led_SetRelayStatus(0x01 << temp, RELAY_OFF);
			return 0;
		}

		//for switch simulator
		if(strcmp(argv[1], "led") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on"))
				sw_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off"))
				sw_SetRelayStatus(0x01 << temp, RELAY_OFF);
			return 0;
		}

		//add new load for apt
		if(strcmp(argv[1], "add") == 0) {
			if (argc < 11) {
				printf("Lack of parameters!\n");
				return 0;
			}
			if (strlen(argv[2]) >= 16) {
				printf("The name is too long!\n");
				return 0;
			}
			strcpy(newload.load_name, argv[2]);
			for (i = 0; i < 8; i++) {
				sscanf(argv[3 + i], "%x", &temp);
				newload.load_ram[i] = (unsigned char)temp;
			}
			if (c131_AddLoad(&newload) == 0) {
				printf("Add new load successfully!\n");
			} else {
				printf("Add new load unsuccessfully!\n");
				return 0;
			}
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_c131 = {"c131", cmd_c131_func, "APT C131 board cmd"};
DECLARE_SHELL_CMD(cmd_c131)
