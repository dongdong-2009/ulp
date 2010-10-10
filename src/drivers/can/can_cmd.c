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

void can_msg_print(can_msg_t *msg, char *str)
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

static int cmd_can_func(int argc, char *argv[])
{
	int x;
#ifdef CONFIG_CAN_CMD_QUEUE
	int ms;
#endif
	can_cfg_t cfg = CAN_CFG_DEF;
	can_msg_t msg;
	const char *usage = {
		"usage:\n"
		"can init ch baud		init can hw interface, def to CH1+500K\n"
		"can send id d0 ...		can send, 11bit id\n"
		"can sene id d0 ...		can send, 29bit id\n"
		"can recv			can bus monitor\n"
		"can qedit ms id d0 ...		can queue edit, 11bit id\n"
		"can qrun			run can queue now\n"
	};

	if(argc > 1) {
		can_flag = 1;
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
		
		if (!strcmp(argv[1], "qrun")) { //queue run
			can_flag = 1;
			timer = time_get(0);
			return can_queue_run();
		}
#endif
		timer = time_get(0);
	}

	if(can_queue_run())
		return 1;
	
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
