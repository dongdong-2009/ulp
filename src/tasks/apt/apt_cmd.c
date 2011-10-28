/*
 * david@2011 initial version
 */
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "stm32f10x.h"
#include "shell/cmd.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "c131_driver.h"
#include "c131_diag.h"
#include "c131.h"

static apt_load_t newload;
static dtc_t dtc_buffer[32];
static c131_dtc_t c131_dtc;
static can_msg_t apt_msg_buf[64];		//for multi frame buffer

static int cmd_apt_func(int argc, char *argv[])
{
	int temp, i, data_len, msg_len, * pdata;
	can_msg_t msg_req;

	const char * usage = { \
		" usage:\n" \
		" apt pwr on/off sdm,  config sdm power on/off \n" \
		" apt add load_name b0 b1 ... b7,  add new config \n" \
		" apt set load_name b0 b1 ... b7,  set current config \n" \
		" apt clr/read dtc, clear the product DTC information \n" \
		" apt diag loop/switch, get diagnose data \n" \
		" apt req id b0..b7, send diag request" \
		" apt eeprom dlen id b0..b7, send eeprom request with data len\n" \
	};

	if (argc > 1) {
		//add new load for apt
		if(strcmp(argv[1], "add") == 0) {
			if (argc < 11) {
				printf("##ERROR##\n");
				return 0;
			}
			if (strlen(argv[2]) >= 16) {
				printf("##ERROR##\n");
				return 0;
			}
			strcpy(newload.load_name, argv[2]);
			for (i = 0; i < 8; i++) {
				sscanf(argv[3 + i], "%x", &temp);
				newload.load_ram[i] = (unsigned char)temp;
			}
			if (apt_AddLoad(&newload) == 0) {
				printf("##OK##\n");
			} else {
				printf("##ERROR##\n");
				return 0;
			}
		}

		//set load for apt
		if (strcmp(argv[1], "set") == 0) {
			if (argc < 11) {
				printf("##ERROR##\n");
				return 0;
			}
			if (strlen(argv[2]) >= 16) {
				printf("##ERROR##\n");
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
					printf("##OK##\n");
				} else if (strcmp(argv[2], "off") == 0) {
					Disable_SDMPWR();
					// Disable_LEDPWR();
					printf("##OK##\n");
				}
			}
			return 0;
		}

		//for reading & clearing dtc
		if (strcmp(argv[2], "dtc") == 0) {
			if(strcmp(argv[1], "read") == 0) {
				//for dtc related varible init
				c131_dtc.pdtc = dtc_buffer;
				if (c131_GetDTC(&c131_dtc))
					printf("##ERROR##\n");
				else {
					printf("##OK##\n");
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
				if (c131_ClearHistoryDTC())
					printf("##ERROR##\n");
				else
					printf("##OK##\n");
			}
		}

		//for geting diagnosis data
		if (strcmp(argv[1], "diag") == 0) {
			if (strcmp(argv[2], "loop") == 0){
				c131_GetDiagLoop(&pdata, &data_len);
				for (i = 0; i < data_len; i++) {
					// printf("Loop%d resistance is %d (mohm)\n", i, pdata[i]);
					printf("Loop%d resistance is %d (mohm)\n", i, pdata[i] - 1000);
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

		/*for diagnostic and eeprom information*/
		if (strcmp(argv[1], "req") == 0) {
			msg_req.dlc = argc - 3;
			if (msg_req.dlc != 8)
				return 0;
			sscanf(argv[2], "%x", &msg_req.id);		//id
			msg_req.flag = 0;
			for(i = 0; i < msg_req.dlc; i ++)
				sscanf(argv[3 + i], "%x", (int *)&msg_req.data[i]);

			if (c131_GetDiagInfo(&msg_req, apt_msg_buf, &msg_len))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				for (i = 0; i < msg_len; i++)
					can_msg_print(apt_msg_buf + i, "\n");
			}
		}

		/*for eeprom information*/
		if (strcmp(argv[1], "eeprom") == 0) {
			msg_req.dlc = argc - 4;
			if (msg_req.dlc != 8)
				return 0;
			sscanf(argv[3], "%x", &msg_req.id);		//id
			msg_req.flag = 0;
			for(i = 0; i < msg_req.dlc; i ++)
				sscanf(argv[4 + i], "%x", (int *)&msg_req.data[i]);
			sscanf(argv[2], "%2x", &data_len);		//id

			if (c131_GetEEPROMInfo(&msg_req, apt_msg_buf, (char)data_len, &msg_len))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				for (i = 0; i < msg_len; i++)
					can_msg_print(apt_msg_buf + i, "\n");
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
