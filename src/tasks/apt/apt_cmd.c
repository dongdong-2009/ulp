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
	int temp, i, data_len;
	int * pdata;

	const char * usage = { \
		" usage:\n" \
		" apt pwr on/off sdm/led,  config sdm/led power on/off \n" \
		" apt add load_name b0 b1 ... b7,  add new config \n" \
		" apt set load_name b0 b1 ... b7,  set current config \n" \
		" apt clr/read dtc, clear the product DTC information \n" \
		" apt diag loop/switch, get diagnose data \n" \
	};

	if (argc > 1) {
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
		if (strcmp(argv[1], "set") == 0) {
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
		if (strcmp(argv[1], "pwr") == 0) {
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

		//for reading & clearing dtc
		if (strcmp(argv[2], "dtc") == 0) {
			if(strcmp(argv[1], "read") == 0) {
				//for dtc related varible init
				c131_dtc.pdtc = dtc_buffer;
				if (c131_can_GetDTC(&c131_dtc))
					printf("Reading DTC Error! \n");
				else {
					printf("Reading DTC Successful! \n");
					if(c131_dtc.dtc_bExist) {
						printf("HB  , MB  , LB  , SODTC\n");
						for (i = 0; i < c131_dtc.dtc_len; i++)
							printf("0x%2x, 0x%2x, 0x%2x, 0x%2x \n", \
							c131_dtc.pdtc[i].dtc_hb, c131_dtc.pdtc[i].dtc_mb, \
							c131_dtc.pdtc[i].dtc_lb, c131_dtc.pdtc[i].dtc_status);
					} else {
						printf("No DTC Exist\n");
					}
				}
			} 

			if(strcmp(argv[1], "clr") == 0) {
				if (c131_can_ClearHistoryDTC())
					printf("Clear DTC Error! \n");
				else
					printf("Clear DTC Successful! \n");
			}
		}

		//for geting diagnosis data
		if (strcmp(argv[1], "diag") == 0) {
			if (strcmp(argv[2], "loop") == 0){
				c131_GetDiagLoop(&pdata, &data_len);
				for (i = 0; i < data_len; i++) {
					printf("Loop%d resistance is %d (mohm)\n", i, pdata[i]);
				}
			}

			if (strcmp(argv[2], "switch") == 0) {
				c131_GetDiagSwitch(&pdata, &data_len);
				//for switch1
				printf("sw1 stage1 is :%d (mA)\n", pdata[0]);
				printf("sw1 stage2 is :%d (mA)\n", pdata[1]);
				printf("sw2 stage1 is :%d (ohm)\n", pdata[2]);
				printf("sw2 stage2 is :%d (ohm)\n", pdata[3]);
				printf("sw3 stage1 is :%d (ohm)\n", pdata[4]);
				printf("sw3 stage2 is :%d (ohm)\n", pdata[5]);
				printf("sw4 stage1 is :%d (ohm)\n", pdata[6]);
				printf("sw4 stage2 is :%d (ohm)\n", pdata[7]);
				printf("sw5 stage1 is :%d (ohm)\n", pdata[8]);
				printf("sw5 stage2 is :%d (ohm)\n", pdata[9]);
			}
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_apt = {"apt", cmd_apt_func, "apt box cmd"};
DECLARE_SHELL_CMD(cmd_apt)
