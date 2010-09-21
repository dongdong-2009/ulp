/*
 *	miaofng@2010 can debugger initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"

static const can_bus_t *can_bus;

static int cmd_can_func(int argc, char *argv[])
{
	int x;
	can_cfg_t cfg = CAN_CFG_DEF;
	can_msg_t msg;
	const char *usage = {
		"usage:\n"
		"can init ch baud		init can hw interface\n"
		"can send id d0 ...		can send, 11bit id\n"
		"can sene id d0 ...		can send, 29bit id\n"
		"can recv			can bus monitor\n"
	};

	if(argc > 1) {
		if(argv[1][0] == 'i') { //can init
			sscanf(argv[2], "%d", &x); //ch
			sscanf(argv[3], "%d", &cfg.baud); //baud
			can_bus = NULL;
#ifdef CONFIG_DRIVER_CAN1
			//default to can1
			if(x == 1)
				can_bus = &can1;
#endif
#ifdef CONFIG_DRIVER_CAN2
			if(x == 2)
				can_bus = &can2;
#endif
			if(can_bus != NULL)
				can_bus -> init(&cfg);
			else
				printf("can interface doesn't exist\n");

			return 0;
		}

		if(argv[1][0] == 's') {//can send/t
			msg.dlc = argc - 3;
			msg.flag = (argv[1][3] == 'e') ? 0 : CAN_FLAG_EXT;
			sscanf(argv[2], "%x", &msg.id); //id
			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[3 + x], "%x", (int *)&msg.data[x]);
			}

			if(can_bus -> send(&msg)) {
				printf("can send fail\n");
			}
			return 0;
		}
	}

	//can recv, monitor
	if(argc == 0 || argv[1][3] == 'v') {
		if(!can_bus -> recv(&msg)) {
			//printf can frame
			if(msg.flag & CAN_FLAG_EXT)
				printf("R%08x ", msg.id);
			else
				printf("R%03x ", msg.id);
			for(x = 0; x < msg.dlc; x ++) {
				printf("%02x ", msg.data[x]);
			}
			printf("\n");
		}
		return 1;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_can = {"can", cmd_can_func, "can monitor/debugger"};
DECLARE_SHELL_CMD(cmd_can)
