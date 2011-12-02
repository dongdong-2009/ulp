/*
 *	miaofng@2011 initial version
 *	king@2011 rewrite
 */
#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "sys/task.h"
#include "nvm.h"
#include "sis.h"
#include "sss.h"
#include "priv/mcamos.h"
#include "sis_card.h"
#include <string.h>
#include "shell/cmd.h"
#include "led.h"
#include "psi5.h"

static enum {
	SIS_STM_INIT,
	SIS_STM_READY,
	SIS_STM_RUN, //normal run
	SIS_LEARN_INIT, //learn mode
	SIS_LEARN_UPDATE,
	SIS_STM_ERR,
} sis_stm;
static int learn_protocol = SIS_PROTOCOL_INVALID;
static int learn_result = SIS_PROTOCOL_INVALID;
static time_t sis_timer = 0;
static mcamos_srv_t sis_server;
static struct sis_sensor_s sis_sensor __nvm;
static enum {
	SIS_EVENT_NONE,
	SIS_EVENT_LEARN,
} sis_event = SIS_EVENT_NONE;

static void sis_run_init()
{
	switch(sis_sensor.protocol) {
	case SIS_PROTOCOL_DBS:
		dbs_init(&sis_sensor.dbs, NULL);
		break;
	case SIS_PROTOCOL_PSI5:
		psi5_init(&sis_sensor.psi5, NULL);
		break;
	}
}

static void sis_run_update()
{
	switch(sis_sensor.protocol) {
	case SIS_PROTOCOL_DBS:
		dbs_update();
		break;
	case SIS_PROTOCOL_PSI5:
		psi5_update();
		break;
	}
}

static void sis_learn_init()
{
	switch(learn_protocol) {
	case SIS_PROTOCOL_DBS:
		dbs_learn_init();
		break;
	case SIS_PROTOCOL_PSI5:
		psi5_learn_init();
		break;
	}
}

static void sis_learn_update()
{
	switch(learn_protocol) {
	case SIS_PROTOCOL_INVALID:
		learn_protocol = SIS_PROTOCOL_DBS;
		sis_stm = SIS_LEARN_INIT;
		break;
	case SIS_PROTOCOL_DBS:
		dbs_learn_update();
		if(dbs_learn_finish()) {
			struct dbs_sensor_s sensor;
			if(!dbs_learn_result(&sensor)) {
				learn_result = SIS_PROTOCOL_DBS;
				sis_stm = SIS_STM_INIT;
				learn_protocol = SIS_PROTOCOL_DBS;
			}
			else {
				sis_stm = SIS_LEARN_INIT;
				learn_protocol = SIS_PROTOCOL_PSI5;
			}
			return;
		}
		break;
	case SIS_PROTOCOL_PSI5:
		psi5_learn_update();
		if(psi5_learn_finish()) {
			struct psi5_sensor_s sensor;
			if(!psi5_learn_result(&sensor))
				learn_result = SIS_PROTOCOL_PSI5;
			else
				learn_result = SIS_PROTOCOL_INVALID;
			sis_stm = SIS_STM_INIT;
			learn_protocol = SIS_PROTOCOL_INVALID;
			return;
		}
		break;
	}
}

static int sis_learn_finish()
{
	switch(learn_protocol) {
	case SIS_PROTOCOL_INVALID:
		return (psi5_learn_finish() || dbs_learn_finish());
	case SIS_PROTOCOL_DBS:
		return 0;
	case SIS_PROTOCOL_PSI5:
		return psi5_learn_finish();
	}
	return 0;
}

static int sis_learn_result(struct sis_sensor_s *sensor)
{
	int ret = -1;
	switch(learn_result) {
	case SIS_PROTOCOL_DBS:
		ret = dbs_learn_result(&sensor->dbs);
		sensor->protocol = SIS_PROTOCOL_DBS;
		break;
	case SIS_PROTOCOL_PSI5:
		ret = psi5_learn_result(&sensor->psi5);
		sensor->protocol = SIS_PROTOCOL_PSI5;
		break;
	default:
		break;
	}
	return ret;
}

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
		if(sis_learn_finish()) {
			sis = (struct sis_sensor_s *)(inbox + 1);
			memcpy(&new, sis, sizeof(new));
			ret = sis_learn_result(&new);
			if(!ret) { //success
				new.cksum = 0;
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
	sis_timer = time_get(500);
}

static int cmd_sis_func(int argc, char *argv[])
{
	if(argc == 2) {
		if(!strcmp(argv[1], "learn")) {
			sis_event = SIS_EVENT_LEARN;
		}
		else if(!strcmp(argv[1], "save")) {
			nvm_save();
		}
		else if(!strcmp(argv[1], "clear")) {
			memset(&sis_sensor, 0, sizeof(sis_sensor));
			sis_sensor.protocol = SIS_PROTOCOL_INVALID;
		}
	}
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
		if((!sis_sum(&sis_sensor, sizeof(sis_sensor))) && (sis_sensor.protocol != SIS_PROTOCOL_INVALID))
			sis_stm = SIS_STM_READY;
		if(sis_event == SIS_EVENT_LEARN) {
			sis_event = SIS_EVENT_NONE;
			sis_stm = SIS_LEARN_INIT;
		}
		break;
	case SIS_STM_READY:
		led_on(LED_GREEN);
		if(time_left(sis_timer) < 0)
			led_flash(LED_RED);
		else
			led_off(LED_RED);
		if(card_getpower()) {
			sis_run_init();
			sis_stm = SIS_STM_RUN;
			led_flash(LED_GREEN);
		}
		if(sis_event == SIS_EVENT_LEARN) {
			sis_event = SIS_EVENT_NONE;
			sis_stm = SIS_LEARN_INIT;
		}
		break;
	case SIS_STM_RUN:
		if(time_left(sis_timer) < 0)
			led_flash(LED_RED);
		else
			led_off(LED_RED);
		sis_run_update();
		if(!card_getpower()) {
			card_player_stop();
			sis_stm = SIS_STM_READY;
		}
		break;
	case SIS_LEARN_INIT:
		sis_learn_init();
		sis_stm = SIS_LEARN_UPDATE;
		break;
	case SIS_LEARN_UPDATE:
		sis_learn_update();
		if(sis_learn_finish())
			sis_stm = SIS_STM_INIT;
#if 1
			sis_learn_result(&sis_sensor);
			sis_sensor.cksum = 0;
			sis_sensor.cksum = 0 - sis_sum(&sis_sensor, sizeof(sis_sensor));
#endif
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
