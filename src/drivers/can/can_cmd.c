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
#include "linux/list.h"
#include "FreeRTOS.h"

#define CONFIG_CAN_CMD_QUEUE

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

static time_t timer;
static const can_bus_t *can_bus;
static char can_flag = 0;

void can_msg_print(const can_msg_t *msg, char *str)
{
	int i;
	printf("ID = %04X: ", msg -> id);
	for( i = 0; i < msg -> dlc; i ++) {
		printf("%02x ", msg -> data[i]);
	}
	
	if(str)
		printf("%s", str);
}

#ifdef CONFIG_CAN_CMD_QUEUE
static LIST_HEAD(can_queue);

int can_queue_add(int ms, can_msg_t *msg)
{
	struct list_head *pos;
	struct can_queue_s *new, *q = NULL;
	
	//prepare new queue unit
	new = MALLOC(sizeof(struct can_queue_s));
	if(new == NULL) {
		printf("error: out of memory!\n");
		return 0;
	}
	
	new -> timer = 0;
	new -> ms = ms;
	memcpy(&new -> msg, msg, sizeof(can_msg_t));
	INIT_LIST_HEAD(&new -> list);
	
	//check current queue, find the id?
	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> msg.id == msg -> id) { //found one
			list_replace(pos, &new -> list);
			FREE(q);
			return 0;
		}
	}
	
	list_add(&new -> list, &can_queue);
	return 0;
}

int can_queue_del(int id)
{
	struct list_head *pos, *n;
	struct can_queue_s *q = NULL;

	//check current queue, find the id?
	list_for_each_safe(pos, n, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> msg.id == id) {
			list_del(&q -> list);
			FREE(q);
		}
	}

	return 0;
}

void can_queue_print(void)
{
	struct list_head *pos;
	struct can_queue_s *q;
	
	printf("queue list:\n");
	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		can_msg_print(&q -> msg, 0);
		printf("T = %03dms\n", q -> ms);
	}
}

void can_queue_clear(void)
{
	struct list_head *pos;
	struct can_queue_s *q;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		FREE(q);
	}
	
	INIT_LIST_HEAD(&can_queue);
}

int can_queue_run(void)
{
	struct list_head *pos;
	struct can_queue_s *q;

	if(!can_flag)
		return 0;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			q -> timer = time_get(q -> ms);
			
			//send the msg now
			printf("%06dms ", (int)((time_get(0) - timer)*1000/CONFIG_TICK_HZ));
			if(can_bus -> send(&q -> msg)) {
				can_msg_print(&q -> msg, " ..fail\n");
			}
			else {
				can_msg_print(&q -> msg, "\n");
			}
		}
	}
	
	return 1;
}
#endif

static const can_msg_t msg1 = {
	0x727, 0x08, {0x10, 0x0b, 0xba, 0x10, 0x17, 0xff, 0x00, 0x00}, 0,
};

static const can_msg_t msg2 = {
	0x727, 0x08, {0x21, 0x00, 0x00, 0x00, 0x00, 0x26, 0xff, 0xff}, 0,
};

static const can_msg_t msg3 = {
	0x727, 0x08, {0x30, 0x08, 0x28, 0xff, 0xff, 0xff, 0xff, 0xff}, 0,
};

void can_bpclr(void)
{
	can_msg_t msg;
	
	//1st message
	can_msg_print(&msg1, "\n");
	if (can_bus -> send(&msg1)) {
		printf("can send fail\n");
		return;
	}
	
	while (1) {
		if(!can_bus -> recv(&msg) && (msg.id == 0x7a7)) {
			can_msg_print(&msg, "\n");
			if (msg.data[0] == 0x30) {
				break;
			}
		}
	}
	
	//2nd message
	can_msg_print(&msg2, "\n");
	if (can_bus -> send(&msg2)) {
		printf("can send fail\n");
		return;
	}
	
	while (1) {
		if(!can_bus -> recv(&msg) && (msg.id == 0x7a7)) {
			can_msg_print(&msg, "\n");
			if (msg.data[0] == 0x10) {
				break;
			}
		}
	}
	
	//3rd message
	can_msg_print(&msg3, "\n");
	if (can_bus -> send(&msg3)) {
		printf("can send fail\n");
		return;
	}
	
	while (1) {
		if(!can_bus -> recv(&msg) && (msg.id == 0x7a7)) {
			can_msg_print(&msg, "\n");
			if (msg.data[0] == 0x21) {
				break;
			}
		}
	}
	
	printf("clear over\n");
	return;
}




static int cmd_can_func(int argc, char *argv[])
{
	int x;
#ifdef CONFIG_CAN_CMD_QUEUE
	int ms;
#endif
	can_cfg_t cfg = CAN_CFG_DEF;
	can_msg_t msg;
	static time_t send_overtime;
	static int can_filter[10];
	static int recv_filter_falg = 0;
	static int filter_count = 0;
	const char *usage = {
		"usage:\n"
		"can init ch baud		init can hw interface, def to CH1+500K\n"
		"can send id d0 ...		can send, 11bit id\n"
		"can sene id d0 ...		can send, 29bit id\n"
		"can recv id0 id1		can bus monitor, id0.. for filter\n"
		"can recv cancel			can bus monitor, cancel filter setting\n"
		"can qedit ms id d0 ...		can queue edit, 11bit id\n"
		"can qdel id			can queue del\n"
		"can qrun			run can queue now\n"
		"can bpclr			clear burn panel fault status log\n"
	};

	if(argc > 1) {
		can_flag = 0;
		if(argv[1][0] == 'i') { //can init
#ifdef CONFIG_CAN_CMD_QUEUE
			can_queue_clear();
#endif
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
			msg.flag = (argv[1][3] == 'd') ? 0 : CAN_FLAG_EXT;

			if (argc > 3)
				sscanf(argv[2], "%x", &msg.id); //id
			else 
				return 0;

			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[3 + x], "%x", (int *)&msg.data[x]);
			}

			if (can_bus -> send(&msg)) {
				printf("can send fail\n");
			} else {
				send_overtime = time_get(1000);
				while (time_left(send_overtime)) {
					if (!can_bus -> recv(&msg)) {
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
				}
			}
			
			return 0;
		}

#ifdef CONFIG_CAN_CMD_QUEUE
		if (!strcmp(argv[1], "qedit")) { //queue edit
			if(argc < 4) {
				can_queue_print();
				return 0;
			}

			msg.dlc = argc - 4;
			if(msg.dlc > 8) { 
				msg.dlc = 8;
				printf("warnning: msg is too long!!!\n");
			}
			msg.flag = 0;
			sscanf(argv[2], "%d", &ms); //id
			sscanf(argv[3], "%x", &msg.id); //id

			for(x = 0; x < msg.dlc; x ++) {
				sscanf(argv[4 + x], "%x", (int *)&msg.data[x]);
			}

			can_queue_add(ms, &msg);
			can_queue_print();

			return 0;
		}

		if (!strcmp(argv[1], "qdel")) { //queue edit
			if(argc < 3) {
				can_queue_print();
				return 0;
			}

			sscanf(argv[2], "%x", &msg.id); //id

			can_queue_del(msg.id);
			can_queue_print();
			return 0;
		}

		if (!strcmp(argv[1], "qrun")) { //queue run
			can_flag = 1;
			timer = time_get(0);
			return can_queue_run();
		}
#endif

		if (!strcmp(argv[1], "bpclr")) { //queue run
			can_bpclr();
			return 0;
		}

		timer = time_get(0);
	}

	if(can_queue_run())
		return 1;

	//can recv, monitor
	if(argc == 0 || argv[1][3] == 'v') {
		if (argc > 2) {
			if (argv[2][5] == 'l') {		//for can recv cancel
				recv_filter_falg = 0;
				return 0;
			}
			else
				recv_filter_falg = 1;
			filter_count = argc - 2;
			for(x = 0; x < filter_count; x++) {
				sscanf(argv[2 + x], "%x", (int *)&can_filter[x]);
			}
		}

		if(!can_bus -> recv(&msg)) {
			if (recv_filter_falg) {
				for(x = 0; x < filter_count; x++) {
					if (can_filter[x] == msg.id)
						break;
					if (x == filter_count - 1)
						return 1;
				}
			}

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
