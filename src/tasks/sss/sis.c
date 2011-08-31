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

static enum {
	SIS_STM_INIT,
	SIS_STM_READY,
	SIS_STM_RUN, //normal run
	SIS_STM_LEARN, //learn mode
	SIS_STM_ERR,
} sis_stm;
static mcamos_srv_t sis_server;
static struct sis_sensor_s sis_sensor __nvm;

char sis_sum(void *p, int n)
{
	char sum, *str = (char *) p;
	for(sum = 0; n > 0; n --) {
		sum += *str;
		str ++;
	}
	return sum;
}

static void serv_init(void)
{
	sis_server.can = &can1;
	sis_server.id_cmd = card_getslot() << 1;
	sis_server.id_dat = sis_server.id_cmd + 1;
	mcamos_srv_init(&sis_server);
}

static int serv_update(void)
{
	char finish, ret;
	char *inbox = sis_server.inbox;
	char *outbox = sis_server.outbox + 2;
	int flag_new;
	struct sis_sensor_s *sis, new;

	ret = 0; //successfully
	finish = 1; //finished the cmd
	mcamos_srv_update(&sis_server);
	switch(inbox[0]) {
	case SSS_CMD_QUERY:
		memcpy(outbox, &sis_sensor, sizeof(sis_sensor));
		break;
	case SSS_CMD_SELECT:
		ret = -1;
		if((sis_stm == SIS_STM_READY) || (sis_stm == SIS_STM_INIT)) {
			flag_new = 0;
			sis = (struct sis_sensor_s *)(inbox + 1);
			if(!sis_sum(sis, sizeof(struct sis_sensor_s))) {
				ret = 0;
				flag_new = memcmp(&sis_sensor, sis, sizeof(sis_sensor));
				memcpy(&sis_sensor, sis, sizeof(sis_sensor));
				if(flag_new)
					nvm_save();
			}
		}
		break;
	case SSS_CMD_LEARN:
		sis = (struct sis_sensor_s *)(inbox + 1);
		finish = dbs_learn_finish();
		if(finish) {
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
	default:
		break;
	}

	if(finish) {
		sis_server.outbox[0] = inbox[0];
		sis_server.outbox[1] = ret;
		inbox[0] = 0; //clear inbox testid indicate cmd ops finished!
	}
	return inbox[0];
}

static void sis_init(void)
{
	card_init();
	serv_init();
	sis_stm = SIS_STM_INIT;
	if(!sis_sum(&sis_sensor, sizeof(sis_sensor))) {
		sis_stm = SIS_STM_READY;
	}
}

static void sis_update(void)
{
	int event = serv_update();
	switch(sis_stm) {
	case SIS_STM_INIT:
		break;
	case SIS_STM_READY:
		if(card_getpower()) { //handle power-up event
			dbs_init(&sis_sensor.dbs, NULL); //only dbs protocol implemented
			sis_stm = SIS_STM_RUN;
		}
		if(event == SSS_CMD_LEARN) {
			dbs_learn_init();
			sis_stm = SIS_STM_LEARN;
		}
		break;
	case SIS_STM_RUN:
		dbs_update();
		if(!card_getpower())
			dbs_poweroff();
		break;
	case SIS_STM_LEARN:
		dbs_learn_update();
		if(dbs_learn_finish()) {
			sis_stm = SIS_STM_READY;
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
