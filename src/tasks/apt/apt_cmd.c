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
	int temp, i, data_len, msg_len, * pdata, * pflag;
	can_msg_t msg_req;
	apt_load_t *pload;

	const char * usage = { \
		" usage:\n" \
		" apt pwr on/off sdm,  config sdm power on/off \n" \
		" apt add load_name b0 b1 ... b7,  add new config \n" \
		" apt set load_name b0 b1 ... b7,  set current config \n" \
		" apt get cfg , get current config \n" \
		" apt clr/read dtc, clear the product DTC information \n" \
		" apt diag loop/switch, get diagnose data \n" \
		" apt req id b0..b7, send diag request\n" \
		" apt eeprom dlen id b0..b7, send eeprom request with data len\n" \
	};

	if (argc > 1) {
		//get current load for apt
		if(strcmp(argv[1], "get") == 0) {
			if (argc > 3 || strcmp(argv[2], "cfg") != 0) {
				printf("##ERROR##\n");
				return 0;
			}
			int index_load = c131_GetCurrentLoadIndex();
			if( c131_GetLoad(&pload,index_load) ==0){
				int i =0;
				while(i<8)
				printf("%x ",pload->load_ram[i++]);
				printf("\n");
			}
			else
				printf("No cfg\n");
		}

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
				if (!Get_SDMStatus() && (strcmp(argv[2], "on") == 0)) {
					Enable_SDMPWR();
					Enable_LEDPWR();
					printf("##OK##\n");
				} else if (strcmp(argv[2], "off") == 0) {
					Disable_SDMPWR();
					Disable_LEDPWR();
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
				c131_GetDiagLoop(&pdata, &pflag, &data_len);
				printf("LOOP    Current      Standard    Status\n");
				for (i = 0; i < data_len; i++) {
					// printf("Loop%d resistance is %d (mohm)\n", i, pdata[i]);
					printf("Loop%2d  %4d (mohm)  2000(mohm)  ", i, pdata[i] - 1000);
					if (pflag[i])
						printf("Error\n");
					else
						printf("OK\n");
				}
			}

			if (strcmp(argv[2], "switch") == 0) {
				c131_GetDiagSwitch(&pdata, &pflag, &data_len);
				//for switch1
				printf("SW  STATE   Current    Standard    Status\n");
				printf("sw1 stage1 :%4d (mA)  14 (mA)     ", pdata[0]);
				if (pflag[0])
					printf("Error\n");
				else
					printf("OK\n");
				printf("sw1 stage2 :%4d (mA)  6  (mA)     ", pdata[1]);
				if (pflag[1])
					printf("Error\n");
				else
					printf("OK\n");
				printf("sw2 stage1 :%4d (ohm) 2820 (ohm)  ", pdata[2]);
				if (pflag[2])
					printf("Error\n");
				else
					printf("OK\n");
				printf("sw2 stage2 :%4d (ohm) 820  (ohm)  ", pdata[3]);
				if (pflag[3])
					printf("Error\n");
				else
					printf("OK\n");

				//for switch3
				if (pflag[4]) {
					printf("sw3 stage1 :%4d (ohm) Short       ", pdata[4]);
					printf("Error\n");
				}  else {
					printf("sw3 stage1 :Short      Short       ");
					printf("OK\n");
				}
				if (pflag[5]) {
					printf("sw3 stage2 :%4d (ohm) Open        ", pdata[5]);
					printf("Error\n");
				} else {
					printf("sw3 stage2 :Open       Open        ");
					printf("OK\n");
				}

				//for switch4
				if (pflag[6]) {
					printf("sw4 stage1 :%4d (ohm) Short       ", pdata[6]);
					printf("Error\n");
				} else {
					printf("sw4 stage1 :Short      Short       ");
					printf("OK\n");
				}
				if (pflag[7]) {
					printf("sw4 stage2 :%4d (ohm) Open        ", pdata[7]);
					printf("Error\n");
				} else {
					printf("sw4 stage2 :Open       Open        ");
					printf("OK\n");
				}

				printf("sw5 stage1 :%4d (ohm) 900 (ohm)   ", pdata[8]);
				if (pflag[8])
					printf("Error\n");
				else
					printf("OK\n");
				printf("sw5 stage2 :%4d (ohm) 220 (ohm)   ", pdata[9]);
				if (pflag[9])
					printf("Error\n");
				else
					printf("OK\n");
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

		//for power off/on
		if (strcmp(argv[1], "can") == 0) {
			if (strcmp(argv[2], "on") == 0) {
                                TIM_Cmd(TIM2, ENABLE);
                                printf("##OK##\n");
			} else if (strcmp(argv[2], "off") == 0) {
				TIM_Cmd(TIM2, DISABLE);
                                printf("##OK##\n");
			}
		return 0;
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
