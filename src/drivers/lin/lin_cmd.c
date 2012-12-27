/*
 *	miaofng@2010 can debugger initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lin.h"
#include "ulp_time.h"
#include "linux/list.h"
#include "sys/sys.h"

enum {
	LIN_NODE_NULL,
	LIN_NODE_RESPONSE,
};

static lin_bus_t *lin_bus;
static int pid_response(unsigned char pid, lin_msg_t *p);

static int pid_response(unsigned char pid, lin_msg_t *p)
{
	int x, result = LIN_SLAVE_TASK_NO_RESPONSE;
	if (pid == 0x38) {
		for(x = 0; x < 8; x ++)
			p->data[x] = x + 0x78;
		p->dlc = 0x08;
		result = LIN_SLAVE_TASK_RESPONSE;
	}
	if (pid == 0x3c) {
		p->data[0] = 0x00;
		for(x = 1; x < 8; x ++)
			p->data[x] = 0xff;
		p->dlc = 0x08;
		result = LIN_SLAVE_TASK_RESPONSE;
	}
	return result;
}

void lin_msg_print(const lin_msg_t *msg, char *str)
{
	int i;
	char h, l;

	printf("ID = 0x%02X ", msg -> pid);
	for( i = 0; i < msg -> dlc; i ++) {
		h = (msg -> data[i] & 0xf0) >> 4;;
		l = msg -> data[i] & 0x0f;
		h = (h <= 9) ? ('0' + h) : ('a' + h - 10);
		l = (l <= 9) ? ('0' + l) : ('a' + l - 10);
		printf("%c%c ", h, l);
	}
	if(str)
		printf("%s", str);
}

static int cmd_lin_func(int argc, char *argv[])
{
	int x, temp;
	lin_cfg_t cfg = LIN_CFG_DEF;
	lin_msg_t msg;
	static time_t send_overtime;
	static unsigned char lin_filter[10];
	static int recv_filter_falg = 0;
	static int filter_count = 0;
	const char *usage = {
		"usage:\n"
		"lin init ch baud m/s   init can hw interface, def to master CH1+9600\n"
		"lin send pid   		lin send pid as master task\n"
		"lin recv id0 id1   	lin bus monitor, id0.. for filter\n"
		"lin recv cancel		can bus monitor, cancel filter setting\n"
	};

	if(argc > 1) {
		if(argv[1][0] == 'i') {
			if(argc >= 3) {
				sscanf(argv[2], "%d", &x); //ch
			} else { //default to channel 1
				x = 1;
			}
			if(argc >= 4) {
				sscanf(argv[3], "%d", &cfg.baud); //baud
			} else { //default to baud 9600
				cfg.baud = 9600;
			}
			if(argc >= 5) {
				if (argv[4][0] == 'm') //master node
					cfg.node = LIN_MASTER_NODE;
				else
					cfg.node = LIN_SLAVE_NODE;
			} else { //default to baud 9600
				cfg.node = LIN_MASTER_NODE;
			}
			cfg.pid_callback = pid_response;
			lin_bus = NULL;
#ifdef CONFIG_DRIVER_LIN0
			if(x == 0)
				lin_bus = &lin0;
#endif
#ifdef CONFIG_DRIVER_LIN1
			if (x == 1)
				lin_bus = &lin1;
#endif
#ifdef CONFIG_DRIVER_LIN2
			if (x == 2)
				lin_bus = &lin2;
#endif
			if(lin_bus != NULL) {
				lin_bus -> init(&cfg);
				printf("Lin bus init ok!\n");
				if (cfg.node == LIN_MASTER_NODE)
					printf("Node: Master\n");
				else
					printf("Node: Slave\n");
				printf("Baud: %d\n", cfg.baud);
			}
			else
				printf("can interface doesn't exist\n");
			return 0;
		}

		if(argv[1][0] == 's') {//lin send
			msg.dlc = argc - 3;
			if (argc == 3) {
				sscanf(argv[2], "%x", &temp); //id
				msg.pid = (unsigned char)temp;
			}
			else
				return 0;
			if (lin_bus -> send(msg.pid)) {
				printf("can send fail\n");
			} else {
				send_overtime = time_get(500);
				while (time_left(send_overtime)) {
					if (!lin_bus -> recv(&msg)) {
						printf("%06dms ", (int)time_get(0));
						//printf lin frame
						lin_msg_print(&msg, "\n");
					}
				}
			}
			return 0;
		}
	}

	//can recv, monitor
	if(argc == 0 || argv[1][3] == 'v') {
		if (argc > 2) {
			if (argv[2][5] == 'l') {		//for can recv cancel
				recv_filter_falg = 0;
				return 0;
			} else {
				recv_filter_falg = 1;
			}
			filter_count = argc - 2;
			for(x = 0; x < filter_count; x++) {
				sscanf(argv[2 + x], "%x", (int *)&lin_filter[x]);
			}
		}

		if(!lin_bus -> recv(&msg)) {
			if (recv_filter_falg) {
				for(x = 0; x < filter_count; x++) {
					if (lin_filter[x] == msg.pid)
						break;
					if (x == filter_count - 1)
						return 1;
				}
			}

			printf("%06d ms ", (int)time_get(0));
			lin_msg_print(&msg, "\n");
		}
		return 1;
	}

	printf("%s", usage);
	return 0;
}

const cmd_t cmd_lin = {"lin", cmd_lin_func, "lin monitor/debugger"};
DECLARE_SHELL_CMD(cmd_lin)

