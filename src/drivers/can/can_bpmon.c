/*
 *	miaofng@2010 can debugger initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"
#include "ulp_time.h"
#include "linux/list.h"
#include "ulp/sys.h"
#include "common/debounce.h"

#define frame_period_threshold 10%

static const can_bus_t *bpmon_can = &can1;
static LIST_HEAD(record_queue);

struct record_s {
	unsigned id;
	time_t time; /*time of last frame*/
	int ms; /*frame period of last time*/
	int ms_min;
	int ms_max;
	int ms_avg;
	struct debounce_s sync;
	struct list_head list;
};

static void *record_get(int id)
{
	struct record_s *record;
	struct list_head *pos;
	list_for_each(pos, &record_queue) {
		record = list_entry(pos, record_s, list);
		if(record->id == id) {
			return record;
		}
	}

	record = sys_malloc(sizeof(struct record_s));
	memset(record, 0, sizeof(struct record_s));
	list_add(&record->list, &record_queue);
	record->id = id;
	debounce_init(&record->sync, 3, 0);
	return record;
}

static int bpmon_init(void)
{
	struct list_head *pos, *n;
	struct record_s *q = NULL;

	//free old records's memory
	list_for_each_safe(pos, n, &record_queue) {
		q = list_entry(pos, record_s, list);
		list_del(&q->list);
		sys_free(q);
	}

	INIT_LIST_HEAD(&record_queue);
	return 0;
}

static void bpmon_update(void)
{
	can_msg_t msg;
	struct record_s *record;

	//frame lost detection
	struct list_head *pos, *bak;
	list_for_each_safe(pos, bak, &record_queue) {
		record = list_entry(pos, record_s, list);
		if(record->sync.on) {
			int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100;
			if(time_left(record->time) < - ms_threshold) {
				printf("%08d : %03xh timeout(T = %dmS[%dms, %dms])\n", time_get(0), record->id, record->ms, record->ms_min, record->ms_max);
				record->time = time_shift(record->time, record->ms_avg);
				if(debounce(&record->sync, 0)) { //can frame lost
					printf("%08d : %03xh !!! is lost(T = %dmS)\n", time_get(0), record->id, record->ms_avg);
					list_del(&record->list);
					sys_free(record);
				}
			}
		}
	}

	if(!bpmon_can -> recv(&msg)) {
		record = record_get(msg.id);
		if((record->sync.off) && (record->ms == 0)) { //1st, 2nd frame
			if(record->time != 0) {
				record->ms = - time_left(record->time);
				record->ms_avg = record->ms;
				debounce(&record->sync, 1);
			}
			record->time = time_get(record->ms_avg);
			return;
		}

		int ms = time_left(record->time); //estimation err
		record->ms -= ms;
		record->time = time_get(record->ms_avg);

		ms = (ms > 0) ? ms : - ms;
		int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100;
		if((ms < ms_threshold) && (record->ms > 0)) {
			record->ms_avg = (record->ms_avg + record->ms) / 2;
			if(debounce(&record->sync, 1)){
				//new can frame is sync, print it ...
				printf("%08d : %03xh !!!", time_get(0), record->id);
				for(int i = 0; i < msg.dlc; i ++) {
					printf(" %02x", ((unsigned)(msg.data[i])) & 0xff);
				}
				printf(" (T = %dmS) is sync\n", record->ms_avg);
			}
			if(record->sync.on) { //update statistics info
				if(record->ms_min == 0) record->ms_min = record->ms;
				else record->ms_min = (record->ms < record->ms_min)?record->ms : record->ms_min;
				record->ms_max = (record->ms > record->ms_max)?record->ms : record->ms_max;
			}
		}
		else {
			if(record->sync.on) {
				//peculiar can frame received, print it ...
				printf("%08d : %03xh", time_get(0), record->id);
				for(int i = 0; i < msg.dlc; i ++) {
					printf(" %02x", ((unsigned)(msg.data[i])) & 0xff);
				}
				printf(" (T = %dmS[%dms,%dms])\n", record->ms, record->ms_min, record->ms_max);
			}
			if(debounce(&record->sync, 0)) { //sync err, resync is needed
				printf("%08d : %03xh !!! sync err, resync ..(T = %dmS)\n", time_get(0), record->id, record->ms_avg);
				list_del(&record->list);
				sys_free(record);
			}
		}
	}
}

static can_cfg_t cfg = {.baud = 500000, .silent = 1};
static can_filter_t *filter = NULL;
static int cmd_bpmon_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bpmon [baud] [158h] [..]	default to 500Kbaud & unmask all can id\n"
	};

	if(argc > 0) {
		int index, id, para_err = 0;
		index = 1;
		if(argv[1][strlen(argv[1])-1] != 'h') {
			cfg.baud = (argc > 1) ? atoi(argv[1]) : 500000;
			para_err += (cfg.baud <= 100) ? 1 : 0;
			index ++;
		}

		if(filter != NULL) {
			sys_free(filter);
			filter = NULL;
		}
		if(index < argc) {
			filter = sys_malloc(sizeof(can_filter_t)*(argc - index));
			assert(filter != NULL);
			for(int i = 0; i < (argc - index); i ++) {
				sscanf(argv[i + index], "%xh", &id);
				filter[i].id = id;
				filter[i].mask = -1;
				para_err += (id <= 0) ? 1 : 0;
				para_err += (id >= 0xffff) ? 1 : 0; /*only std id is supported yet*/
			}
		}
		if(para_err) {
			printf("%s", usage);
			return 0;
		}

		bpmon_can->init(&cfg);
		if(filter != NULL) {
			bpmon_can->filt(filter, argc - index);
		}

		bpmon_init();
		return 1;
	}

#if 0
	//voltage monitor
	int mv = bpmon_measure(0);
	if(bpmon_power.v12on == 0) {
		if(mv > 3000) {
			bpmon_power.v12cnt ++;
			if(bpmon_power.v12cnt > 3) {
				bpmon_power.v12on = 1;
				bpmon_power.v12cnt = 0;
			}
		}
		else {
			if(bpmon_power.v12cnt > 0) {
				bpmon_power.v12cnt --;
			}
		}
	}
	else {
		if(mv < 9000) {
			bpmon_power.v12cnt ++;
			if(bpmon_power.v12cnt > 3) {
				bpmon_power.v12on = 0;
				bpmon_power.v12cnt = 0;
			}
		}
		else {
			if(bpmon_power.v12cnt > 0) {
				bpmon_power.v12cnt --;
			}
		}
	}
#endif
	bpmon_update();
	return 1;
}

const cmd_t cmd_bpmon = {"bpmon", cmd_bpmon_func, "can monitor for bpanel"};
DECLARE_SHELL_CMD(cmd_bpmon)
