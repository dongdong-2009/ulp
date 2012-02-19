/*
 *	sky@2011 initial version
 *	peng.guo@2011 modify
 *	miaofng@2011 rewrite
 *	king@2011 modify
 *
 */
#include "config.h"
#include "sys/task.h"
#include "shell/cmd.h"
#include "priv/mcamos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulp_time.h"
#include "sss.h"
#include "sis.h"
#include "nvm.h"
#include "led.h"
#include "ulp/debug.h"

#define SSS_MAGIC 0x56784321
#define SSS_LIST_SIZE	64

static struct sis_sensor_s sss_list[SSS_LIST_SIZE] __nvm;
static unsigned sss_magic __nvm;
static char sss_mailbox[64];

/*search the specified sensor in list, or find an empty one if name == NULL*/
static struct sis_sensor_s *sss_search(const char *name)
{
	int i;
	struct sis_sensor_s *sis = NULL;

	for(i = 0; i < SSS_LIST_SIZE; i ++) {
		sis = &sss_list[i];
		if((sis->protocol != 0) && (!sis_sum(sis, sizeof(struct sis_sensor_s)))) {
			if((name != NULL) && (!strncmp(sis->name, name, 13))) {
				break;
			}
		}
		else {
			if(name == NULL) {
				sis = &sss_list[i];
				break;
			}
		}
		sis = NULL;
	}

	return sis;
}

static int mcamos_wait(int ms)
{
	char mailbox;
	int ret = -1;
	time_t deadline = time_get(ms);
	do {
		if(mcamos_upload_ex(MCAMOS_INBOX_ADDR, &mailbox, 1))
			break;

		if(mailbox == 0) {
			ret = 0;
			break;
		}
	} while(time_left(ms) > 0);
	return ret;
}

static int cmd_sss_list(const char *name)
{
	int i, ret = -1;
	struct sis_sensor_s *sis;

	for(i = 0; i < SSS_LIST_SIZE; i ++) {
		sis = &sss_list[i];
		if((sis->protocol != 0) && (!sis_sum(sis, sizeof(struct sis_sensor_s)))) {
			if(name == NULL) {
				printf("sensor %02d:	%s;\n", i, sis->name);
				ret = 0;
			}
			else if(!strcmp(name, sis->name)) {
				sis_print(sis);
				ret = 0;
			}
		}
	}

	if(ret) {
		if(name != NULL) {
			printf("sensor not found");
			ret = -1;
		}
		else {
			printf("no sensor in the list");
			ret = -2;
		}
	}

	return ret;
}

static int cmd_sss_config(const char *name, const char *para)
{
	int n;
	struct sis_sensor_s *sis = NULL;
	struct sis_sensor_s new;

	assert(name != NULL);
	sis = sss_search(name);
	if(para == NULL) { //del
		if(sis != NULL) {
			memset(sis, 0, sizeof(struct sis_sensor_s));
			nvm_save();
			return 0;
		}
		else {
			printf("sensor not found");
			return -1;
		}
	}

	if(sis == NULL) {
		sis = sss_search(NULL);
		if(sis == NULL) {
			printf("sss list full");
			return -2;
		}
	}

	memset(&new, 0, sizeof(new));
	strncpy(new.name, name, 13);
	sscanf(para, "0x%x",&n);
	switch(n) {
	case SIS_PROTOCOL_PSI5:
		int data[23];
		n = sscanf(para, "0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,\
			0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",\
			&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6],\
			&data[7], &data[8], &data[9], &data[10], &data[11], &data[12], &data[13],\
			&data[14], &data[15], &data[16], &data[17], &data[18], &data[19], &data[20],\
			&data[21], &data[22]\
			);
		if(n == 23) {
			new.protocol = (char)data[0];
			for(int i = 0; i < 22; i++)
				new.data[i] = (char)data[i + 1];
			new.cksum = 0 - sis_sum(&new, sizeof(new));
			memcpy(sis, &new, sizeof(new));
			nvm_save();
			return 0;
		}
		printf("para not enough");
		return -3;
	case SIS_PROTOCOL_DBS:
		int temp[11];
		n = sscanf(para, "0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,\
			0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",\
			&temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &temp[6],\
			&temp[7], &temp[8], &temp[9], &temp[10]\
			);
		if(n == 11) {
			new.protocol = (char)temp[0];
			for(int i = 0; i < 11; i++)
				new.data[i] = (char)temp[i + 1];
			new.cksum = 0 - sis_sum(&new, sizeof(new));
			memcpy(sis, &new, sizeof(new));
			nvm_save();
			return 0;
		}
		printf("para not enough");
		return -3;
	default:
		printf("This protocol is not exist");
		return -1;
	}
}

static int cmd_sss_select(const char *name, int bdn)
{
	int ret;
	struct sis_sensor_s *sis;
	struct mcamos_s m = {
		.can = &can1,
		.id_cmd = sss_GetID(bdn),
		.id_dat = sss_GetID(bdn) + 1,
		.timeout = 50,
	};

	assert(name != NULL);
	sis = sss_search(name);
	if(sis == NULL) {
		printf("sensor not found");
		return -1;
	}

	//communication with the sis board
	mcamos_init_ex(&m);
	ret = mcamos_wait(0);
	if(ret) {
		printf("board %d not exist", bdn);
		return -1;
	}
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR + 1, sis, sizeof(struct sis_sensor_s));
	sss_mailbox[0] = SSS_CMD_SELECT;
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, sss_mailbox, 1);
	ret += mcamos_wait(100);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR, sss_mailbox, 2);
	if(ret) {
		printf("board %d communication error", bdn);
		return -2;
	}

	if(sss_mailbox[1]) {
		printf("board %d sensor not supported", bdn);
		return -3;
	}

	return 0;
}

static int cmd_sss_query(int bdn, int echo)
{
	int ret;
	struct sis_sensor_s sis;
	struct mcamos_s m = {
		.can = &can1,
		.id_cmd = sss_GetID(bdn),
		.id_dat = sss_GetID(bdn) + 1,
		.timeout = 50,
	};

	//communication with the sis board
	mcamos_init_ex(&m);
	ret = mcamos_wait(0);
	if(ret) {
		if(echo) printf("board %d not exist", bdn);
		return -1;
	}

	sss_mailbox[0] = SSS_CMD_QUERY;
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, sss_mailbox, 1);
	ret += mcamos_wait(100);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR, sss_mailbox, 2 + sizeof(sis));
	if(ret) {
		if(echo) printf("board %d communication error", bdn);
		return -2;
	}

	if(sss_mailbox[1]) {
		if(echo) printf("board %d sensor not configured", bdn);
		return -3;
	}

	memcpy(&sis, &sss_mailbox[2], sizeof(sis));
	if(echo) printf("board %02d: %s\n", bdn, sis.name);
	if(echo) sis_print(&sis);
	return 0;
}

static int cmd_sss_learn(const char *name, int bdn)
{
	int ret;
	struct sis_sensor_s sis, *p;
	struct mcamos_s m = {
		.can = &can1,
		.id_cmd = sss_GetID(bdn),
		.id_dat = sss_GetID(bdn) + 1,
		.timeout = 50,
	};

	memset(&sis, 0, sizeof(sis));
	//sis.protocol = SIS_PROTOCOL_DBS;
	strcpy(sis.name, name);

	//communication with the sis board
	mcamos_init_ex(&m);
	ret = mcamos_wait(0);
	if(ret) {
		printf("board %d not exist", bdn);
		return -1;
	}

	//send learn cmd to sis card
	sss_mailbox[0] = SSS_CMD_LEARN;
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, sss_mailbox, 1);

	//try to read the answer, may fail(learn busy)
	ret += mcamos_wait(100);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR, sss_mailbox, 2);
	if((ret == 0) && (sss_mailbox[1] != 0)) {
		printf("operation refused");
		return -5;
	}

	//sis card is busy on learn
	mdelay(800);

	sss_mailbox[0] = SSS_CMD_LEARN_RESULT;
	memcpy(&sss_mailbox[1], &sis, sizeof(sis));
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, sss_mailbox, sizeof(sis) + 1);
	ret += mcamos_wait(100);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR, sss_mailbox, 2 + sizeof(sis));
	if(ret) {
		printf("board %d communication error", bdn);
		return -2;
	}

	if(sss_mailbox[1]) {
		printf("board %d sensor cann't identified", bdn);
		return -3;
	}

	//printf
	memcpy(&sis, &sss_mailbox[2], sizeof(sis));
	printf("board %02d found ", bdn);
	sis_print(&sis);

	//save to nvm
	p = sss_search(name);
	p = (p == NULL) ? sss_search(NULL) : p;
	if(p == NULL) {
		printf("sss list full");
		return -4;
	}

	memcpy(p, &sis, sizeof(sis));
	nvm_save();
	return 0;
}

static int cmd_sss_save(int bdn, int clear)
{
	int ret;
	struct mcamos_s m = {
		.can = &can1,
		.id_cmd = sss_GetID(bdn),
		.id_dat = sss_GetID(bdn) + 1,
		.timeout = 50,
	};

	//communication with the sis board
	mcamos_init_ex(&m);
	ret = mcamos_wait(0);
	if(ret) {
		printf("board %d not exist", bdn);
		return -1;
	}

	sss_mailbox[0] = (clear) ? SSS_CMD_CLEAR : SSS_CMD_SAVE;
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, sss_mailbox, 1);
	mdelay(500); //sis card are busy on nvm_save() now ...
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR, sss_mailbox, 2);
	if(ret) {
		printf("board %d communication error", bdn);
		return -2;
	}

	return 0;
}

static int cmd_sss_func(int argc, char *argv[])
{
	int ret = -1;
	const char *usage = {
		"usage:\n"
		"sss learn name <bdn>			add/modify new sensor automatically\n"
		"sss config name protocol,data0,data1,..	add/modify/remove new sensor manually\n"
		"sss select name <bdn>			select a sensor to be emulated, bdn is optional\n"
		"sss query <bdn>				query specified board sensor info\n"
		"sss list <name>				list all supported sensors\n"
		"sss save/clear <bdn>			save/clear sis board current configuration\n"
	};

	if((argc >= 2) && (!strcmp(argv[1], "list"))) {
		char *name = (argc > 2) ? argv[2] : NULL;
		ret = cmd_sss_list(name);
	}

	else if((argc >= 3) && (!strcmp(argv[1], "config"))) {
		char *para = (argc > 3) ? argv[3] : NULL;
		ret = cmd_sss_config(argv[2], para);
	}

	else if((argc >= 3) && (!strcmp(argv[1], "select"))) {
		unsigned pat, i;
		pat = (argc > 3) ? cmd_pattern_get(argv[3]) : 0xff << 1;
		ret = (pat <= 1) ? -1 : 0;
		for(i = 1; i <= 31; i ++) {
			if(pat & (1 << i)) {
				ret = cmd_sss_select(argv[2], i);
				if(ret)
					break;
			}
		}
	}

	else if((argc >= 2) && (!strcmp(argv[1], "query"))) {
		int pat, i;
		pat = (argc > 2) ? cmd_pattern_get(argv[2]) : 0xff << 1;
		ret = (pat <= 1) ? -1 : 0;
		for(i = 1; i <= 31; i ++) {
			if(pat & (1 << i)) {
				ret = cmd_sss_query(i, 1);
				if(ret)
					break;
			}
		}
	}

	else if((argc >= 3) && (!strcmp(argv[1], "learn"))) {
		int pat, i;
		pat = (argc > 3) ? cmd_pattern_get(argv[3]) : 0x02;
		ret = (pat <= 1) ? -1 : 0;
		for(i = 1; i <= 31; i ++) {
			if(pat & (1 << i)) {
				ret = cmd_sss_learn(argv[2], i);
				if(ret)
					break;
			}
		}
	}

	else if((argc >= 2) && (!strcmp(argv[1], "save"))) {
		int pat, i;
		pat = (argc > 2) ? cmd_pattern_get(argv[2]) : 0xff << 1;
		ret = (pat <= 1) ? -1 : 0;
		for(i = 1; i <= 31; i ++) {
			if(pat & (1 << i)) {
				ret = cmd_sss_save(i, 0);
				if(ret)
					break;
			}
		}
	}

	else if((argc >= 2) && (!strcmp(argv[1], "clear"))) {
		int pat, i;
		pat = (argc > 2) ? cmd_pattern_get(argv[2]) : 0xff << 1;
		ret = (pat <= 1) ? -1 : 0;
		for(i = 1; i <= 31; i ++) {
			if(pat & (1 << i)) {
				ret = cmd_sss_save(i, 1);
				if(ret)
					break;
			}
		}
	}

	else if((argc == 1) || (!strcmp(argv[1], "help")))
		printf("%s", usage);

	if(ret == 0)
		printf("##OK##\n");
	else
		printf("##%d##ERROR##\n", ret);

	return 0;
}

const cmd_t cmd_sss = {"sss", cmd_sss_func, "can monitor/debugger"};
DECLARE_SHELL_CMD(cmd_sss)

static void sss_Init(void)
{
	led_on(LED_GREEN);
	led_off(LED_RED);
	if(sss_magic != SSS_MAGIC) {
		sss_magic = SSS_MAGIC;
		memset(sss_list, 0, sizeof(sss_list));
		nvm_save();
	}
}

//poll each board healthy status
static char sss_slot = 1;
static int sss_health = 0xffff;
static void sss_Update(void)
{
	int ret, mask;

	mask = 1 << sss_slot;
	ret = cmd_sss_query(sss_slot, 0);
	if(ret && ret != -3) {
		if(sss_health & mask) {
			printf("warnning: sis board %d fail(%d)!!!\n", sss_slot, ret);
			sss_health &= ~(1 << sss_slot);
		}
	}
	else {
		if(!(sss_health & mask)) {
			printf("warnning: sis board %d getting better!!!\n", sss_slot);
			sss_health |= 1 << sss_slot;
		}
	}

	if((sss_health >> 1) & 0xff != 0xff) {
		led_off(LED_GREEN);
		led_flash(LED_RED);
	}
	else {
		led_off(LED_RED);
		led_on(LED_GREEN);
	}
	sss_slot = (sss_slot < 8) ? sss_slot + 1 : 1;
}

//#define SSS_DEBUG_LOOP_TIME
#ifdef SSS_DEBUG_LOOP_TIME
static time_t sss_loop_timer;
#endif

int main(void)
{
	task_Init();
	sss_Init();
	mdelay(300);
	while(1) {
#ifdef SSS_DEBUG_LOOP_TIME
		printf("loop = %dmS\n", - time_left(sss_loop_timer));
		sss_loop_timer = time_get(0);
#endif
		task_Update();
		sss_Update();
	}
}
