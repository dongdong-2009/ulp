/*
 * David@2011 init version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell/cmd.h"
#include "c131_relay.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "mbi5025.h"

static int cmd_c131_func(int argc, char *argv[])
{
	int temp = 0;
	int bd, ch, bus;
	int i;
	char simulator_data[4];

	const char * usage = { \
		" usage:\n" \
		" matrix info,         printf the board information \n" \
		" matrix con nbus nch, matrix connect a point \n" \
		" matrix dis nbus nch, matrix disconnect a point \n" \
		" matrix on,           connect all relays \n" \
		" matrix off,          disconnect all relays \n" \
		" matrix channel nch,  connect all (8) relays on nch\n" \
		" matrix bus nbus,     connect all (46) relays on nbus\n" \
	};

	if (argc > 1) {
		if(strcmp(argv[1], "info") == 0) {
			for (i = 0; i < CONFIG_SLOT_NR; i++) {
				if (bdi[i].bExist == 0) {
					printf(" Board %d don't exist \n\n", i);
				} else {
					printf(" Board %d info as follow:\n", i);
					printf(" Shift register chain length: %d\n", bdi[i].iLen);
					printf(" Number of channel on board: %d\n", bdi[i].iNrChannel);
					printf(" Number of bus on board: %d\n", bdi[i].iNrBus);
					printf(" Bus switch exist status: %d\n", bdi[i].bBusSWExist);
					printf(" Channel switch exist status: %d\n\n", bdi[i].bChSWExist);
				}
			}
			return 0;
		}

		//connect infomation
		if(strcmp(argv[1], "con") == 0) {
			simulator_data[0] = (char)MATRIX_CMD_CONNECT;
			simulator_data[1] = 1;
			sscanf(argv[2], "%d", &temp);
			simulator_data[2] = (char)temp;
			sscanf(argv[3], "%d", &temp);
			simulator_data[3] = (char)temp;
			temp = matrix_handler(MATRIX_CMD_CONNECT, simulator_data);
			if (temp != ERROR_OK) {
				printf("Operation Error! Error Code is : %d\n", temp);
			}
			return 0;
		}

		//disconnection infomation
		if(strcmp(argv[1], "dis") == 0) {
			simulator_data[0] = (char)MATRIX_CMD_DISCONNECT;
			simulator_data[1] = 1;
			sscanf(argv[2], "%d", &temp);
			simulator_data[2] = (char)temp;
			sscanf(argv[3], "%d", &temp);
			simulator_data[3] = (char)temp;
			temp = matrix_handler(MATRIX_CMD_DISCONNECT, simulator_data);
			if (temp != ERROR_OK) {
				printf("Operation Error! Error Code is : %d\n", temp);
			}
			return 0;
		}

		if(strcmp(argv[1], "on") == 0) {
			for (bd = 0; bd < CONFIG_SLOT_NR; bd++) {
				if (bdi[bd].bExist == 1) {
					matrix_BoardSelect(bd);
					for (i = 46; i >= 0; i--) {
						BoardBuf[bd][i] = 0xff;
						mbi5025_WriteByte(&sr, BoardBuf[bd][i]);
					}
					BOARD_SR_LoadEable();
					BOARD_SR_LoadDisable();
				}
			}
			return 0;
		}

		if(strcmp(argv[1], "off") == 0) {
			for (bd = 0; bd < CONFIG_SLOT_NR; bd++) {
				if (bdi[bd].bExist == 1) {
					matrix_BoardSelect(bd);
					for (i = 46; i >= 0; i--) {
						BoardBuf[bd][i] = 0x00;
						mbi5025_WriteByte(&sr, BoardBuf[bd][i]);
					}
					BOARD_SR_LoadEable();
					BOARD_SR_LoadDisable();
				}
			}
			return 0;
		}

		//channel on
		if(strcmp(argv[1], "channel") == 0) {
			sscanf(argv[2], "%d", &temp);
			bd = (int )(temp / 46);
			ch = temp % 46;
			matrix_BoardSelect(bd);
			BoardBuf[bd][ch] = 0xff;
			BoardBuf[bd][46] = 0xff;
			for(i = 46; i >= 0; i--) {
				mbi5025_WriteByte(&sr, BoardBuf[bd][i]);
			}
			BOARD_SR_LoadEable();
			BOARD_SR_LoadDisable();
			return 0;
		}

		//bus on
		if(argv[1][0] == 'b') {
			sscanf(argv[2], "%d", &bus);
			for (bd = 0; bd < CONFIG_SLOT_NR; bd++) {
				if (bdi[bd].bExist == 1) {
					matrix_BoardSelect(bd);
					temp = (0x01 << bus);
					for (i = 46; i >= 0; i--) {
						BoardBuf[bd][i] |= (unsigned char)temp;
						mbi5025_WriteByte(&sr, BoardBuf[bd][i]);
					}
					BOARD_SR_LoadEable();
					BOARD_SR_LoadDisable();
				}
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
const cmd_t cmd_c131 = {"c131", cmd_c131_func, "APT C131 board cmd"};
DECLARE_SHELL_CMD(cmd_c131)
#endif

