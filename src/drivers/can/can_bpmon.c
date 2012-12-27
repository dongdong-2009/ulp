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
static struct {
	unsigned power : 1;
} bpmon_flag;
static struct debounce_s pwr_12v;
static struct debounce_s pwr_5v;
static LIST_HEAD(record_queue);

struct record_s {
	unsigned id;
	time_t time; /*time of last frame*/
	int ms; /*frame period of last time*/
	int ms_min;
	int ms_max;
	int ms_avg;
	unsigned fsn; //frame sequency nr
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
	bpmon_can->flush();
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
			int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100 + 5;
			if(time_left(record->time) < - ms_threshold) {
				printf("%08d : %03xh timeout\n", time_get(0), record->id);
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
		record->fsn ++;
		if(record->fsn == 1) { // 1st frame
			record->time = time_get(0);
			return;
		}

		if(record->fsn == 2) {
			record->ms = - time_left(record->time);
			if(record->ms > 0) {
				record->ms_avg = record->ms;
				record->time = time_get(record->ms_avg);
			}
			else {
				record->fsn = 0; //restart
			}
			return;
		}

		int ms = record->ms_avg - time_left(record->time); //newly calculated period
		if(ms <= 0) { //useless, strange ... shouldn't be here!!!
			return;
		}

		record->ms = ms;
		record->time = time_get(record->ms_avg);

		ms = ms - record->ms_avg;
		ms = (ms > 0) ? ms : - ms;
		int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100 + 5;
		if(ms < ms_threshold) {
			record->ms_avg = (record->ms_avg + record->ms) / 2;
			if(record->sync.off) {
				printf("%08d : %03xh", time_get(0), record->id);
				for(int i = 0; i < msg.dlc; i ++) {
					printf(" %02x", ((unsigned)(msg.data[i])) & 0xff);
				}
				printf(" is in range(%dmS ?= %dmS)\n", record->ms, record->ms_avg);
			}
			if(debounce(&record->sync, 1)){
				//new can frame is sync, print it ...
				printf("%08d : %03xh !!!(T = %dmS) is sync\n", time_get(0), record->id, record->ms_avg);
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
				printf(" (T = %dmS[%dms,%dms]) .. peculiar\n", record->ms, record->ms_min, record->ms_max);
				if(debounce(&record->sync, 0)) { //sync err, resync is needed
					printf("%08d : %03xh !!! sync err, resync ..(T = %dmS)\n", time_get(0), record->id, record->ms_avg);
					list_del(&record->list);
					sys_free(record);
				}
			}
			else {	//try to sync with new frame period
				record->fsn = 0;
			}
		}
	}
}

#include "stm32f10x.h"
#define ADC_CH_12V ADC_Channel_15 //PC5
#define ADC_CH_5V ADC_Channel_14 //PC4
static void bpmon_measure_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ADC_DeInit(ADC1);
	ADC_DeInit(ADC2);
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_CH_12V, 1, ADC_SampleTime_7Cycles5);
	ADC_RegularChannelConfig(ADC2, ADC_CH_5V, 1, ADC_SampleTime_7Cycles5);

	ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
	ADC_ExternalTrigInjectedConvConfig(ADC2, ADC_ExternalTrigInjecConv_None);
	ADC_ExternalTrigInjectedConvCmd(ADC2, ENABLE);

	ADC_Cmd(ADC1, ENABLE);
	ADC_Cmd(ADC2, ENABLE);
	ADC_StartCalibration(ADC1);
	ADC_StartCalibration(ADC2);
	while (ADC_GetCalibrationStatus(ADC1));
	while (ADC_GetCalibrationStatus(ADC2));

	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	ADC_ClearFlag(ADC2, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(ADC2, ENABLE);
}

static int bpmon_measure(int ch)
{
	ADC_TypeDef* adc = (ch == ADC_CH_12V) ? ADC1 : ADC2;
	while(!ADC_GetFlagStatus(adc, ADC_FLAG_EOC));
	int mv, digi = ADC_GetConversionValue(adc);
	mv = digi * 3300 / 65536 * 11;
	ADC_ClearFlag(adc, ADC_FLAG_EOC);
	ADC_SoftwareStartConvCmd(adc, ENABLE);
	return mv;
}

static can_cfg_t cfg = {.baud = 500000, .silent = 1};
static can_filter_t *filter = NULL;
static int cmd_bpmon_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bpmon			monitor can frame with baud = 500000 and all can id\n"
		"bpmon 158h 039h [...]	monitor can frame with id 0x158, 0x039, ...\n"
		"bpmon 500000 158h	monitor can with baudrate = 500000\n"
		"bpmon pwrd 500000 158h	monitor can with power detection on\n"
	};

	if(argc > 0) {
		int index = 1, id, para_err = 0;
		bpmon_flag.power = 0;
		for(;index < argc; index ++) {
			if(!strcmp(argv[index], "pwrd")) { //enable power on detection
				bpmon_flag.power = 1;
				debounce_init(&pwr_12v, 10, 0);
				debounce_init(&pwr_5v, 300, 0);
				bpmon_measure_init();
			}
			else if(argv[index][strlen(argv[1])-1] != 'h') {
				cfg.baud = (argc > 1) ? atoi(argv[1]) : 500000;
				para_err += (cfg.baud <= 100) ? 1 : 0;
			}
			else break;
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

	if(bpmon_flag.power) {
		int mv = bpmon_measure(ADC_CH_12V);
		//printf("%08d : 12V = %dmV\n", time_get(0), mv);
		if(debounce(&pwr_12v, (mv > 8000))) {
			printf("%08d : !!!12V (%dmV) is power %s\n", time_get(0), mv, (pwr_12v.on)?"up" : "down");
			if(pwr_12v.on) {
				bpmon_init();
			}
		}

		mv = bpmon_measure(ADC_CH_5V);
		//printf("%08d : 5V = %dmV\n", time_get(0), mv);
		if(debounce(&pwr_5v, (mv > 4500))) {
			printf("%08d : !!!5V (%dmV) is power %s\n", time_get(0), mv, (pwr_5v.on)?"up" : "down");
			if(pwr_5v.on) {
				printf("####################################\n");
				bpmon_init();
			}
		}
	}

	if((bpmon_flag.power == 0) || (pwr_5v.on && pwr_12v.on))
		bpmon_update();
	return 1;
}


const cmd_t cmd_bpmon = {"bpmon", cmd_bpmon_func, "can monitor for bpanel"};
DECLARE_SHELL_CMD(cmd_bpmon)

static int cmd_bpstatus_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bpstatus	display bpmonitor status\n"
	};

	if(argc > 1) {
		if(!strcmp(argv[1], "help")) {
			printf("%s", usage);
			return 0;
		}
	}

	int mv = bpmon_measure(ADC_CH_12V);
	printf("%08d : 12V = %dmV\n", time_get(0), mv);

	mv = bpmon_measure(ADC_CH_5V);
	printf("%08d : 5V = %dmV\n", time_get(0), mv);

	struct record_s *record;
	if((bpmon_flag.power == 0) || (pwr_5v.on && pwr_12v.on)) {
		//frame lost detection
		struct list_head *pos;
		list_for_each(pos, &record_queue) {
			record = list_entry(pos, record_s, list);
			printf("%08d : %03xh is %s(T = %dmS[%dms, %dms])\n", time_get(0), record->id, (record->sync.on)?"sync":"lost", record->ms_avg, record->ms_min, record->ms_max);
		}
		printf("\n");
	}
	return 0;
}

const cmd_t cmd_bpstatus = {"bpst", cmd_bpstatus_func, "bpmon status display"};
DECLARE_SHELL_CMD(cmd_bpstatus)
