/*
 *	miaofng@2011 initial version
 */
#include "config.h"
#include "debug.h"
#include "time.h"
#include "sys/task.h"
#include "nvm.h"
#include "sis.h"
#include "sss.h"
#include "priv/mcamos.h"
#include "sis_card.h"
#include <string.h>
#include "shell/cmd.h"
#include "led.h"

static enum {
	SIS_STM_INIT,
	SIS_STM_READY,
	SIS_STM_RUN, //normal run
	SIS_LEARN_INIT, //learn mode
	SIS_LEARN_UPDATE,
	SIS_STM_ERR,
} sis_stm;
static mcamos_srv_t sis_server;
static struct sis_sensor_s sis_sensor __nvm;
static enum {
	SIS_EVENT_NONE,
	SIS_EVENT_LEARN,
} sis_event = SIS_EVENT_NONE;

static void serv_init(void)
{
	sis_server.can = &can1;
	sis_server.id_cmd = sss_GetID(card_getslot());
	sis_server.id_dat = sis_server.id_cmd + 1;
	mcamos_srv_init(&sis_server);
}

static void serv_update(void)
{
	char ret = -1;
	char *inbox = sis_server.inbox;
	char *outbox = sis_server.outbox + 2;
	struct sis_sensor_s *sis, new;

	mcamos_srv_update(&sis_server);
	switch(inbox[0]) {
	case SSS_CMD_NONE:
		return;
	case SSS_CMD_QUERY:
		memcpy(outbox, &sis_sensor, sizeof(sis_sensor));
		if((!sis_sum(&sis_sensor, sizeof(sis_sensor))) && (sis_sensor.protocol != SIS_PROTOCOL_INVALID))
			ret = 0;
		break;
	case SSS_CMD_SELECT:
		ret = -1;
		if((sis_stm == SIS_STM_READY) || (sis_stm == SIS_STM_INIT)) {
			sis = (struct sis_sensor_s *)(inbox + 1);
			if(!sis_sum(sis, sizeof(struct sis_sensor_s))) {
				ret = 0;
				memcpy(&sis_sensor, sis, sizeof(sis_sensor));
			}
		}
		break;
	case SSS_CMD_LEARN:
		if((sis_stm == SIS_STM_READY) || (sis_stm == SIS_STM_INIT)) {
			sis_event = SIS_EVENT_LEARN;
			ret = 0;
		}
		break;
	case SSS_CMD_LEARN_RESULT:
		ret = -2; //op refused
		if(dbs_learn_finish()) {
			sis = (struct sis_sensor_s *)(inbox + 1);
			memcpy(&new, sis, sizeof(new));
			ret = dbs_learn_result(&new.dbs);
			if(!ret) { //success
				new.cksum = 0;
				new.protocol = SIS_PROTOCOL_DBS;
				new.cksum = 0 - sis_sum(&new, sizeof(new));
				memcpy(outbox, &new, sizeof(new));
			}
		}
		break;
	case SSS_CMD_CLEAR:
		memset(&sis_sensor, 0, sizeof(sis_sensor));
		sis_sensor.protocol = SIS_PROTOCOL_INVALID;
	case SSS_CMD_SAVE:
		ret = 0;
		nvm_save();
		break;
	default:
		ret = -1; //unsupported command
		break;
	}

	sis_server.outbox[0] = inbox[0];
	sis_server.outbox[1] = ret;
	sis_server.inbox[0] = 0; //!!!clear inbox testid indicate cmd ops finished
}

static int cmd_sis_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"sis select name	select a sensor by name\n"
		"sis learn		learn a new sensor, and print it's para\n"
	};

	if(argc == 3) {
		if(!strcmp(argv[1], "select")) {
			int sensor = 0;
			const char trace[3][10] = {
				{0x01, 0x01, 0x06, 0x20, 0x00, 0x44, 0x00, 0x50, 0x71, 0x83},
				{0x01, 0x02, 0x08, 0x20, 0x00, 0x73, 0x93, 0x20, 0x71, 0x90},
				{0x01, 0x02, 0x02, 0x20, 0x09, 0x2e, 0x03, 0x62, 0x13, 0x34},
			};

			if(sscanf(argv[2], "%d", &sensor) > 0) {
				if((sensor < 3) && (sensor >= 0)) {
					memset(&sis_sensor, 0, sizeof(sis_sensor));
					sis_sensor.protocol = SIS_PROTOCOL_DBS;
					sprintf(sis_sensor.name, "%d", sensor);
					memcpy(&sis_sensor.dbs, trace[sensor], 10);
					sis_sensor.cksum = 0 - sis_sum(&sis_sensor, sizeof(sis_sensor));
					nvm_save();
					return 0;
				}
			}
		}
	}

	if(argc == 2) {
		if(!strcmp(argv[1], "learn")) {
			sis_event = SIS_EVENT_LEARN;
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}
const cmd_t cmd_sis = {"sis", cmd_sis_func, "sis debug commands"};
DECLARE_SHELL_CMD(cmd_sis)

static void sis_init(void)
{
	card_init();
	serv_init();
	sis_stm = SIS_STM_INIT;
}

static void sis_update(void)
{
	serv_update();
	switch(sis_stm) {
	case SIS_STM_INIT:
		led_on(LED_GREEN);
		led_on(LED_RED);
		if((!sis_sum(&sis_sensor, sizeof(sis_sensor))) && (sis_sensor.protocol != SIS_PROTOCOL_INVALID)) {
			sis_stm = SIS_STM_READY;
		}
		if(sis_event == SIS_EVENT_LEARN) {
			sis_event = SIS_EVENT_NONE;
			sis_stm = SIS_LEARN_INIT;
		}
		break;
	case SIS_STM_READY:
		led_on(LED_GREEN);
		led_off(LED_RED);
		if(card_getpower()) { //handle power-up event
			dbs_init(&sis_sensor.dbs, NULL); //only dbs protocol implemented
			sis_stm = SIS_STM_RUN;
			led_flash(LED_GREEN);
		}
		if(sis_event == SIS_EVENT_LEARN) {
			sis_event = SIS_EVENT_NONE;
			sis_stm = SIS_LEARN_INIT;
		}
		break;
	case SIS_STM_RUN:
		dbs_update();
		if(!card_getpower()) {
			dbs_poweroff();
			sis_stm = SIS_STM_READY;
		}
		break;
	case SIS_LEARN_INIT:
		dbs_learn_init();
		sis_stm = SIS_LEARN_UPDATE;
		break;
	case SIS_LEARN_UPDATE:
		dbs_learn_update();
		if(dbs_learn_finish()) {
#if 1
			struct dbs_sensor_s sensor;
			if(!dbs_learn_result(&sensor)) {
				printf("\n\ndbs sensor = {\n");
				printf("	.speed = 0x%02x(%dmps)\n", sensor.speed, (1 << (4 - sensor.speed))*1000);
				printf("	.mode = 0x%02x\n", sensor.mode);
				printf("	.addr = 0x%02x\n", sensor.addr);
				for(int i = 0; i < 8; i ++) {
					unsigned x = sensor.trace[i] & 0xff;
					printf("	.trace[%d] = 0x%02x\n", i, x);
				}
				printf("}\n");
			}
			else {
				printf("learn failed\n");
			}
#endif
			sis_stm = SIS_STM_INIT;
		}
		break;
	case SIS_STM_ERR:
		break;
	default:
		break;
	}
}

int main(void)
{
	task_Init();
	sis_init();
	for(;;) {
		task_Update();
		sis_update();
	}
}
