/*
 *	miaofng@2012 bpanel monitor to solve EAT timeout issue
 *	led status:
 *	- green flash: monitor works normal
 *	- red flash: error occurs, specified operation should be taken
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
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "flash.h"
#include "led.h"

char bpmon_line[128];
char bpmon_time_str[32]; //"1/17/2013 18:07:22"
time_t bpmon_time_start;
#define __now__ ((0 - time_left(bpmon_time_start)) % 1000)

#define frame_period_threshold 30%

static const can_bus_t *bpmon_can = &can1;
static struct {
	unsigned baud; //can baudrate
	unsigned kline_baud; //kline baudrate
	unsigned kline_ms : 16;
	unsigned power : 1;
	unsigned kline : 1;
	unsigned verbose : 1;
} bpmon_config;
static struct debounce_s pwr_12v;
static struct debounce_s pwr_5v;
static LIST_HEAD(record_queue);

#define bpmon_log_start 0x08010000 /*provided code size <64K!!!*/
#define bpmon_log_pages (FLASH_PAGE_NR - 40) /*STM32F103RET6: 432K*/

static struct bpmon_log_s {
	unsigned short magic;
	unsigned short rv0;
	unsigned short addr_h;
	unsigned short rv1;
	unsigned short addr_l;
	unsigned short rv2;
	unsigned short end_h;
	unsigned short rv3;
	unsigned short end_l;
	unsigned short rv4;
} *bpmon_log;

void bpmon_log_init(int pages_to_be_erased)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP,ENABLE);
	PWR_BackupAccessCmd(ENABLE); //disable bkp domain write protection
	bpmon_log = (struct bpmon_log_s *) (BKP_BASE + BKP_DR1);

	if(pages_to_be_erased) {
		bpmon_log->magic = 0x1234;
		bpmon_log->addr_h = bpmon_log_start >> 16;
		bpmon_log->addr_l = (short) bpmon_log_start;
		unsigned bpmon_log_end = bpmon_log_start + (pages_to_be_erased * FLASH_PAGE_SZ - 128);
		bpmon_log->end_h = bpmon_log_end >> 16;
		bpmon_log->end_l = (short) bpmon_log_end;
		flash_Erase((char *)bpmon_log_start, pages_to_be_erased);
	}
}

void bpmon_log_print(void)
{
	bpmon_log_init(0);
	if(bpmon_log->magic != 0x1234) {
		printf("bkp not init or data lost??? pls try 'bplog -E'\n");
		return;
	}

	char *end = (char *) bpmon_log->addr_l;
	end += (unsigned)(bpmon_log->addr_h) << 16;
	for(char *p = (char *) bpmon_log_start; p < end; p ++) {
		putchar(*p);
	}
}

void bpmon_log_save(const void *str, int n)
{
	if(bpmon_log->magic != 0x1234) {
		printf("!!! %s: bkp not init or data lost??? pls try 'bplog -E'\n", bpmon_time_str);
		led_off(LED_GREEN);
		led_error(1);
		return;
	}

	char *addr = (char *) bpmon_log->addr_l;
	addr += (unsigned)(bpmon_log->addr_h) << 16;
	char *end = (char *) bpmon_log->end_l;
	end += (unsigned)(bpmon_log->end_h) << 16;

	if(addr < end) {
		flash_Write(addr, str, n);
		addr += n;
		bpmon_log->addr_h = (unsigned)addr >> 16;
		bpmon_log->addr_l = (short)addr;
	}
	else {
		printf("!!! %s: bkp is full, pls try 'bplog -E'\n", bpmon_time_str);
		led_off(LED_GREEN);
		led_error(2);
	}
}

enum info_type {
	BPMON_MORE, //VERBOSE INFO
	BPMON_SYNC,
	BPMON_LOST,
	BPMON_INFO,
	BPMON_MISC,
	BPMON_NULL,
};

int bpmon_print(enum info_type type, const char *fmt, ...)
{
	va_list ap;
	int n = 0;

	//add suffix
	switch(type) {
	case BPMON_LOST:
		led_off(LED_GREEN);
		led_error(3);
	case BPMON_SYNC:
	case BPMON_INFO:
		n += sprintf(bpmon_line, "!!! %s: ", bpmon_time_str);
		break;
	case BPMON_MISC:
	case BPMON_MORE:
		n += sprintf(bpmon_line, "%04d : ", __now__);
		break;
	default:;
	}

	va_start(ap, fmt);
	n += vsnprintf(bpmon_line + n, 128 - n, fmt, ap);
	va_end(ap);

	if(n & 0x03) {
		int odd = 4 - (n & 0x03);
		for(int i = 0; i < odd; i ++) {
			bpmon_line[++ n] = 0;
		}
	}

	//output string to console or log buffer
	printf("%s", bpmon_line);
	static char __type;
	__type = (type != BPMON_MISC) ? type : __type;
	if(__type != BPMON_MORE)
		bpmon_log_save(bpmon_line, n);
	return n;
}

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

	bpmon_time_start = time_get(0);

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
			int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100 + 10;
			if(time_left(record->time) < - ms_threshold) {
				bpmon_print(BPMON_LOST, "%03xh timeout\n", record->id);
				record->time = time_shift(record->time, record->ms_avg);
				if(debounce(&record->sync, 0)) { //can frame lost
					bpmon_print(BPMON_LOST, "%03xh (T=%dmS) is lost\n", record->id, record->ms_avg);
					list_del(&record->list);
					sys_free(record);
				}
			}
		}
	}

	if(!bpmon_can -> recv(&msg)) {
		if(bpmon_config.verbose) {
			bpmon_print(BPMON_MORE, "%03xh", msg.id);
			for(int i = 0; i < msg.dlc; i ++) {
				bpmon_print(BPMON_NULL, " %02x", ((unsigned)(msg.data[i])) & 0xff);
			}
			bpmon_print(BPMON_NULL, "\n");
			return;
		}

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
		int ms_threshold = record->ms_avg * (frame_period_threshold 100) / 100 + 10;
		if(ms < ms_threshold) {
			record->ms_avg = (record->ms_avg + record->ms) / 2;
			if(record->sync.off) {
				bpmon_print(BPMON_MISC, "%03xh (%dmS?=%dmS) is hit", record->id, record->ms, record->ms_avg);
				for(int i = 0; i < msg.dlc; i ++) {
					bpmon_print(BPMON_NULL, " %02x", ((unsigned)(msg.data[i])) & 0xff);
				}
				bpmon_print(BPMON_NULL, "\n");
			}
			if(debounce(&record->sync, 1)){
				//new can frame is sync, print it ...
				bpmon_print(BPMON_SYNC, "%03xh (T=%dmS) is sync\n", record->id, record->ms_avg);
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
				bpmon_print(BPMON_LOST, "%03xh (T=%dmS[%dms,%dms]) is peculiar ..", record->id, record->ms, record->ms_min, record->ms_max);
				for(int i = 0; i < msg.dlc; i ++) {
					bpmon_print(BPMON_NULL, " %02x", ((unsigned)(msg.data[i])) & 0xff);
				}
				bpmon_print(BPMON_NULL, "\n");
				if(debounce(&record->sync, 0)) { //sync err, resync is needed
					bpmon_print(BPMON_LOST, "%03xh (T=%dmS) is lost, resync ..\n", record->id, record->ms_avg);
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

#define AIN_VBAT ADC_Channel_10 //PC0
#define AIN0 ADC_Channel_4//PA4
#define AIN1 ADC_Channel_5//PA5
#define AIN2 ADC_Channel_6//PA6
#define AIN3 ADC_Channel_7//PA7
#define ADC_CH_12V AIN_VBAT
#define ADC_CH_5V AIN0
static void bpmon_measure_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); /*72Mhz/6 = 12Mhz*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
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

#include "uart.h"
static uart_bus_t *bpmon_uart = &uart1;
static struct bpmon_kline_s {
	time_t timer;
	int ms, ms_min, ms_max, ms_avg;
	struct debounce_s sync;
} bpmon_kline;

static void bpmon_kline_init(void)
{
	bpmon_kline.timer = 0;
	bpmon_kline.ms = 0;
	bpmon_kline.ms_min = 0;
	bpmon_kline.ms_max = 0;
	bpmon_kline.ms_avg = 0;
	debounce_init(&bpmon_kline.sync, 3, 0);

	//kline input flush
	while(bpmon_uart->poll()) {
		bpmon_uart->getchar();
	}
}

static void bpmon_kline_update(void)
{
	if(bpmon_uart->poll() == 0) {
		if(bpmon_kline.sync.on) {
			int ms_threshold = bpmon_kline.ms_avg + bpmon_kline.ms_avg * (frame_period_threshold 100) / 100 + 10;
			if(time_left(bpmon_kline.timer) < - ms_threshold) {
				bpmon_kline.timer = time_get(0);
				bpmon_print(BPMON_LOST, "ods (T=%dmS) is timeout\n", bpmon_kline.ms_avg);
				if(debounce(&bpmon_kline.sync, 0)) {
					bpmon_print(BPMON_LOST, "ods (T=%dmS) is lost\n", bpmon_kline.ms_avg);
					bpmon_kline.timer = 0;
				}
			}
		}
		return;
	}

	while(bpmon_uart->poll()) {
		bpmon_uart->getchar();
	}

	if(bpmon_kline.timer == 0) {
		bpmon_kline.timer = time_get(0);
		return;
	}

	int ms = - time_left(bpmon_kline.timer);
	if(ms > bpmon_config.kline_ms) { //a new frame is found
		bpmon_kline.ms = ms;
		bpmon_kline.timer = time_get(0);
		if(bpmon_kline.ms_avg == 0) {
			bpmon_kline.ms_avg = ms;
			return;
		}

		//in expected range???
		ms -= bpmon_kline.ms_avg;
		ms = (ms > 0) ? ms : - ms;
		int ms_threshold = bpmon_kline.ms_avg * (frame_period_threshold 100) / 100 + 10;
		if(ms < ms_threshold) { //good
			if(bpmon_kline.sync.off)
				bpmon_print(BPMON_MISC, "ods (T=%dmS) is hit\n", bpmon_kline.ms);
			bpmon_kline.ms_avg = (bpmon_kline.ms_avg + bpmon_kline.ms) / 2;
			if(debounce(&bpmon_kline.sync, 1)) {
				bpmon_print(BPMON_SYNC, "ods (T=%dmS) is sync\n", bpmon_kline.ms_avg);
			}
			if(bpmon_kline.sync.on) { //update statistics info
				if(bpmon_kline.ms_min == 0) bpmon_kline.ms_min = bpmon_kline.ms;
				else bpmon_kline.ms_min = (bpmon_kline.ms < bpmon_kline.ms_min)?bpmon_kline.ms : bpmon_kline.ms_min;
				bpmon_kline.ms_max = (bpmon_kline.ms > bpmon_kline.ms_max)?bpmon_kline.ms : bpmon_kline.ms_max;
			}
		}
		else {
			if(bpmon_kline.sync.on) { //peculiar kline frame received, print it ...
				bpmon_print(BPMON_LOST, "ods (T=%dmS[%dms,%dms]) is peculiar\n", bpmon_kline.ms, bpmon_kline.ms_min, bpmon_kline.ms_max);
				if(debounce(&bpmon_kline.sync, 0)) { //sync err, resync is needed
					bpmon_print(BPMON_LOST, "ods (T=%dmS) is lost, resync ..\n", bpmon_kline.ms_avg);
					bpmon_kline_init();
				}
			}
			else {	//try to sync with new frame period
				bpmon_kline_init();
			}
		}
	}
}

static int cmd_bpmon_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bpmon			monitor can frame with baud = 500000 and all can id\n"
		"bpmon 158h 039h [...]	monitor can frame with id 0x158, 0x039, ...\n"
		"bpmon -b 500000 158h	monitor can with baudrate = 500000\n"
		"bpmon -k [20] [19200]	monitor can with ods detection, >20ms idle time\n"
		"bpmon -p -k		monitor can with power/ods detection\n"
		"bpmon -v		monitor can with details info\n"
	};

	time_t t;
	time(&t);
	struct tm *now = localtime(&t);
	strftime(bpmon_time_str, 32, "%H:%M:%S", now);

	if(argc > 0) {
		led_flash(LED_GREEN);
		led_off(LED_RED);
		bpmon_log_init(0);
		strftime(bpmon_time_str, 32, "%m/%d/%Y %H:%M:%S", now);
		bpmon_print(BPMON_INFO, "Monitor is Starting ...\n");
		strftime(bpmon_time_str, 32, "%H:%M:%S", now);

		bpmon_config.baud = 500000;
		bpmon_config.power = 0;
		bpmon_config.kline = 0;
		bpmon_config.kline_baud = 19200;
		bpmon_config.kline_ms = 10;
		bpmon_config.verbose = 0;
		can_filter_t *filter = NULL;
		int v, n, e, id = 0;
		for(int i = 1; i < argc; i ++) {
			if(argv[i][strlen(argv[i])-1] == 'h') { //filter id
				n = argc - i;
				filter = sys_malloc(sizeof(can_filter_t)*n);
				assert(filter != NULL);
				for(int j = 0; j < n; j ++) {
					sscanf(argv[i + j], "%xh", &id);
					if(id > 0) {
						filter[j].id = id;
						filter[j].mask = -1;
					}
					else e ++;
				}
				break;
			}
			else e += (argv[i][0] != '-');

			switch(argv[i][1]) {
			case 'p':
				bpmon_config.power = 1;
				break;
			case 'b':
				v = atoi(argv[++i]);
				if(v > 0) bpmon_config.baud = v;
				else e ++;
				break;
			case 'k':
				bpmon_config.kline = 1;
				if((i + 1) < argc && isdigit(argv[i + 1][0])) { // kline ms
					v = atoi(argv[++i]);
					if(v > 0) bpmon_config.kline_ms = v;
					else e ++;
				}
				if((i + 1) < argc && isdigit(argv[i + 1][0])) { // kline baud
					v = atoi(argv[++i]);
					if(v > 0) bpmon_config.kline_baud = v;
					else e ++;
				}
				break;
			case 'v':
				bpmon_config.verbose = 1;
				break;
			default:
				e ++;
			}
		}

		if(e) {
			printf("%s", usage);
			return 0;
		}

		can_cfg_t cfg;
		cfg.baud = bpmon_config.baud;
		cfg.silent = 1;
		bpmon_can->init(&cfg);
		if(filter != NULL) {
			bpmon_can->filt(filter, n);
			sys_free(filter);
			filter = NULL;
		}

		if(bpmon_config.power) {
			debounce_init(&pwr_12v, 10, 0);
			debounce_init(&pwr_5v, 300, 0);
			bpmon_measure_init();
		}

		if(bpmon_config.kline) {
			uart_cfg_t kcfg;
			kcfg.baud = bpmon_config.kline_baud;
			bpmon_uart->init(&kcfg);
			bpmon_kline_init();
		}
		bpmon_init();
		return 1;
	}

	if(bpmon_config.power) {
		int mv = bpmon_measure(ADC_CH_12V);
		//bpmon_print(BPMON_MISC, "12V = %dmV\n", mv);
		if(debounce(&pwr_12v, (mv > 8000))) {
			bpmon_print(BPMON_INFO, "12V (%dmV) is power %s\n", mv, (pwr_12v.on)?"up" : "down");
			if(pwr_12v.on) {
				bpmon_init();
				bpmon_kline_init();
			}
		}

		mv = bpmon_measure(ADC_CH_5V);
		//bpmon_print(BPMON_MISC, "5V = %dmV\n", mv);
		if(debounce(&pwr_5v, (mv > 4500))) {
			bpmon_print(BPMON_INFO, "5V (%dmV) is power %s\n", mv, (pwr_5v.on)?"up" : "down");
			if(pwr_5v.on) {
				bpmon_print(BPMON_NULL, "####################################\n");
				bpmon_init();
				bpmon_kline_init();
			}
		}
	}

	if((bpmon_config.power == 0) || (pwr_5v.on && pwr_12v.on)) {
		bpmon_update();
		if(bpmon_config.kline)
			bpmon_kline_update();
	}
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

	time_t t;
	time(&t);
	struct tm *now = localtime(&t);
	strftime(bpmon_time_str, 32, "%m/%d/%Y %H:%M:%S", now);

	int mv = bpmon_measure(ADC_CH_12V);
	bpmon_print(BPMON_INFO, "12V = %dmV\n", mv);

	//mv = bpmon_measure(ADC_CH_5V);
	bpmon_print(BPMON_INFO, "5V = %dmV\n", mv);

	if(bpmon_config.kline) {
		bpmon_print(BPMON_INFO, "ods (T=%dmS) is %s\n", bpmon_kline.ms_avg, (bpmon_kline.sync.on) ? "sync" : "lost");
	}
	struct record_s *record;
	if((bpmon_config.power == 0) || (pwr_5v.on && pwr_12v.on)) {
		//frame lost detection
		struct list_head *pos;
		list_for_each(pos, &record_queue) {
			record = list_entry(pos, record_s, list);
			bpmon_print(BPMON_INFO, "%03xh (T=%dmS[%dms, %dms]) is %s\n", record->id,
				record->ms_avg, record->ms_min, record->ms_max, (record->sync.on)?"sync":"lost");
		}
		bpmon_print(BPMON_NULL, "\n");
	}
	return 0;
}

const cmd_t cmd_bpstatus = {"bpst", cmd_bpstatus_func, "bpmon status display"};
DECLARE_SHELL_CMD(cmd_bpstatus)

static int cmd_bplog_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"bplog			display recorded messages in flash\n"
		"bplog -E [pages]	erase all recorded messages\n"
		"bplog -s xyz		save xyz to flash\n"
	};

	int v, e = 0;
	if(argc == 1) {
		bpmon_log_print();
		return 0;
	}

	for(int i = 1; i < argc; i ++) {
		e += (argv[i][0] != '-');
		switch(argv[i][1]) {
		case 'E':
			i ++;
			v = (i < argc) ? atoi(argv[i]) : bpmon_log_pages;
			if((v > 0) && (v <= bpmon_log_pages)) {
				printf("Erasing %d pages, pls wait ...\n", v);
				bpmon_log_init(v);
				return 0;
			}

			printf("err: pages is out range(0, %d]\n", bpmon_log_pages);
			e ++;
			break;

		case 's':
			i ++;
			if(i < argc) {
				int n = bpmon_print(BPMON_NULL, "%s\n", argv[i]);
				printf("%d bytes has been saved\n", n);
				return 0;
			}
			e ++;
			break;

		default:
			e ++;
		}
	}

	if(e)
		printf("%s", usage);
	return 0;
}

const cmd_t cmd_bplog = {"bplog", cmd_bplog_func, "bpanel log cmds"};
DECLARE_SHELL_CMD(cmd_bplog)
