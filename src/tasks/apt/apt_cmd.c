/*
 * david@2011 initial version
 */
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "shell/cmd.h"
#include "sys/task.h"
#include "time.h"
#include "c131_driver.h"
#include "c131_diag.h"
#include "c131.h"

static apt_load_t newload;
static dtc_t dtc_buffer[32];
static c131_dtc_t c131_dtc;

static int cmd_apt_func(int argc, char *argv[])
{
	int temp;
	int i;

	const char * usage = { \
		" usage:\n" \
		" apt ess on/off nESS, config nESS on/off \n" \
		" apt loop on/off nLOOP, config nLOOP on/off \n" \
		" apt led on/off nLED, config nLED on/off \n" \
		" apt switch on/off nSWITCH, config nSWITCH on/off \n" \
		" apt pwr on/off sdm/led,  config sdm/led power on/off \n" \
		" apt add load_name b0 b1 ... b7,  add new config \n" \
		" apt set load_name b0 b1 ... b7,  set current config \n" \
		" apt clr dtc, clear the product DTC information \n" \
		" apt read dtc, read the product DTC information \n" \
	};

	// if (apt_GetMode() == C131_MODE_NORMAL){
		// printf("In normal mode\n");
		// return 0;
	// }

	if (argc > 1) {
		//for ess simulator
		if(strcmp(argv[1], "ess") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on") == 0)
				ess_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off") == 0)
				ess_SetRelayStatus(0x01 << temp, RELAY_OFF);
			c131_relay_Update();
			return 0;
		}

		//for loop simulator
		if(strcmp(argv[1], "loop") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on") == 0)
				loop_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off") == 0)
				loop_SetRelayStatus(0x01 << temp, RELAY_OFF);
			c131_relay_Update();
			return 0;
		}

		//for led simulator
		if(strcmp(argv[1], "led") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on") == 0)
				led_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off") == 0)
				led_SetRelayStatus(0x01 << temp, RELAY_OFF);
			c131_relay_Update();
			return 0;
		}

		//for switch simulator
		if(strcmp(argv[1], "switch") == 0) {
			sscanf(argv[3], "%d", &temp);
			if (strcmp(argv[2], "on") == 0)
				sw_SetRelayStatus(0x01 << temp, RELAY_ON);
			if (strcmp(argv[2], "off") == 0)
				sw_SetRelayStatus(0x01 << temp, RELAY_OFF);
			c131_relay_Update();
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
			if (apt_AddLoad(&newload) == 0) {
				printf("Add new load successfully!\n");
			} else {
				printf("Add new load unsuccessfully!\n");
				return 0;
			}
		}

		//set load for apt
		if(strcmp(argv[1], "set") == 0) {
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
			apt_SetAPTRAM(newload.load_ram);
		}

		//for power off/on
		if(strcmp(argv[1], "pwr") == 0) {
			if (strcmp(argv[3], "sdm") == 0) {
				if (strcmp(argv[2], "on") == 0) {
					Enable_SDMPWR();
					Enable_LEDPWR();
				} else if (strcmp(argv[2], "off") == 0) {
					Disable_SDMPWR();
					Disable_LEDPWR();
				}
			}
			if (strcmp(argv[3], "led") == 0) {
				if (strcmp(argv[2], "on") == 0)
					Enable_LEDPWR();
				if (strcmp(argv[2], "off") == 0)
					Disable_LEDPWR();
			}
			return 0;
		}

		//for clearing dtc
		if(strcmp(argv[1], "clr") == 0) {
			if (c131_can_ClearHistoryDTC())
				printf("Clear DTC Error! \n");
			else
				printf("Clear DTC Successful! \n");
		}

		//for reading dtc
		if(strcmp(argv[1], "read") == 0) {
			//for dtc related varible init
			c131_dtc.pdtc = dtc_buffer;
			if (c131_can_GetDTC(&c131_dtc))
				printf("Reading DTC Error! \n");
			else {
				printf("Reading DTC Successful! \n");
				printf("HB  , MB  , LB  , SODTC\n");
				for (i = 0; i < c131_dtc.dtc_len; i++)
					printf("0x%2x, 0x%2x, 0x%2x, 0x%2x \n", \
					c131_dtc.pdtc[i].dtc_hb, c131_dtc.pdtc[i].dtc_mb, \
					c131_dtc.pdtc[i].dtc_lb, c131_dtc.pdtc[i].dtc_status);
			}
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_apt = {"apt", cmd_apt_func, "APT board cmd"};
DECLARE_SHELL_CMD(cmd_apt)
