/*
 *	miaofng@2010 can debugger initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"
#include "time.h"

static time_t timer;
static const can_bus_t *can_bus;

static int cmd_can_func(int argc, char *argv[])
{
	int x;
	can_cfg_t cfg = CAN_CFG_DEF;
	can_msg_t msg;
	const char *usage = {
		"usage:\n"
		"can init ch baud		init can hw interface, def to CH1+500K\n"
		"can send id d0 ...		can send, 11bit id\n"
		"can sene id d0 ...		can send, 29bit id\n"
		"can recv			can bus monitor\n"
	};

	if(argc > 1) {
		timer = time_get(0);
		if(argv[1][0] == 'i') { //can init
			if(argc >= 3) {
				sscanf(argv[2], "%d", &x); //ch
			}
			else { //default to channel 1
				x = 1;
			}
			if(argc >= 4) {
				sscanf(argv[3], "%d", &cfg.baud); //baud
			}
			else { //default to baud 500K
				cfg.baud = 500000;
			}
			
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
			printf("%06dms ", (int)((time_get(0) - timer)*1000/CONFIG_TICK_HZ));
			
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
