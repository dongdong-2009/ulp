/*
 *	David peng.guo@2011 initial version
 */

#include <string.h>
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "can.h"
#include "drv.h"
#include "ls1203.h"
#include "cfg.h"
#include "priv/usdt.h"
#include "ulp/debug.h"
#include "shell/cmd.h"
#include "pdi.h"
#include "stdio.h"


#define PDI_DEBUG	0

//pdi rc-n7 can msg
static const can_msg_t rc_clrdtc_msg =		{0x752, 8, {0x03, 0x3b, 0x9f, 0xff, 0, 0, 0, 0}, 0};
#if PDI_DEBUG
static const can_msg_t rc_errcode_msg =		{0x752, 8, {0x03, 0x21, 0x90, 0x80, 0, 0, 0, 0}, 0};
static const can_msg_t rc_connector_msg =	{0x752, 8, {0x03, 0x21, 0x90, 0xa2, 0, 0, 0, 0}, 0};
static const can_msg_t rc_getsn_msg =		{0x752, 8, {0x03, 0x21, 0x90, 0xab, 0, 0, 0, 0}, 0};
#endif
static const can_msg_t rc_getpart_msg =		{0x752, 8, {0x02, 0x21, 0x83, 0, 0, 0, 0, 0}, 0};
static const can_msg_t rc_reqseed_msg =		{0x752, 8, {0x02, 0x27, 0x61, 0, 0, 0, 0, 0}, 0};
static const can_msg_t rc_start_msg =		{0x752, 8, {0x02, 0x10, 0xc0, 0, 0, 0, 0, 0}, 0};

static const ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 14,
		.dead_time = 20,
};

static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t rc_msg_buf[32];		//for multi frame buffer
static char rc_data_buf[256];			//data buffer
static char rc_fault_buf[64];			//fault buffer
static char bcode_1[14];

//get data from ECU
static int rc_GetCID(short cid, char *data);
static int rc_GetFault(char *data, int * pnum_fault);

/**************************************************************************/
/************         Local funcitons                         *************/
/**************************************************************************/

/*delay ms(ms)*/
int pdi_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			ulp_update();
		}
	} while(left > 0);

	return 0;
}

/*start session*/
int pdi_startsession(void)
{
#if PDI_DEBUG
	int i, num_fault;
#endif
	int msg_len;
	unsigned char seed[2], result;
	can_msg_t msg;
	can_msg_t sendkey_msg = {0x752, 8, {0x04, 0x27, 0x62, 0xff, 0xff, 0, 0, 0}, 0};

	if (usdt_GetDiagFirstFrame(&rc_start_msg, 1, NULL, &msg, &msg_len))		//start session
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif
	if (usdt_GetDiagFirstFrame(&rc_reqseed_msg, 1, NULL, &msg, &msg_len))	//req seed
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

	/*calculate the key from seed*/
	seed[0] = (unsigned char)msg.data[3];
	seed[1] = (unsigned char)msg.data[4];
	result = seed[0] ^ seed[1];
	result ^= 0x34;
	result ^= 0xab;
	sendkey_msg.data[3] = (char)((0x34ab + result) >> 8);
	sendkey_msg.data[4] = (char)((0x34ab + result) & 0x00ff);
	/*send key*/
	if (usdt_GetDiagFirstFrame(&sendkey_msg, 1, NULL, &msg, &msg_len))
		return 1;
	/*judge the send key response*/
	if ((msg.data[1] != 0x67) || (msg.data[2] != 0x62))
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

#if PDI_DEBUG
	/*get serial number*/
	printf("\nSN Code:\n");
	usdt_GetDiagFirstFrame(&rc_getsn_msg, 1, NULL, rc_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(rc_msg_buf, msg_len);
	for (i = 0; i < msg_len; i ++)
		can_msg_print(rc_msg_buf + i, "\n");

	/*get error code*/
	printf("\nError Code:\n");
	usdt_GetDiagFirstFrame(&rc_errcode_msg, 1, NULL, rc_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(rc_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(rc_msg_buf + i, "\n");
	/*tester point*/
	printf("\nPart:\n");
	usdt_GetDiagFirstFrame(&rc_getpart_msg, 1, NULL, rc_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(rc_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(rc_msg_buf + i, "\n");

	if (rc_GetFault(rc_fault_buf, &num_fault))
		printf("##ERROR##\n");
	else {
		printf("##OK##\n");
		printf("num of fault is: %d\n", num_fault);
		for (i = 0; i < num_fault*3; i += 3)
			printf("0x%2x, 0x%2x, 0x%2x\n", rc_fault_buf[i]&0xff, rc_fault_buf[i+1]&0xff, rc_fault_buf[i+2]&0xff);
	}
	/*clear all*/
	printf("\nClear all:\n");
	if (usdt_GetDiagFirstFrame(&rc_clrdtc_msg, 1, NULL, &msg, &msg_len))	//req seed
		return 1;
	can_msg_print(&msg, "\n");
#endif

	return 0;
}

/*get contents of cid,saved to data*/
static int rc_GetCID(short cid, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x752, 8, {0x03, 0x21, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = (char)(cid >> 8);
	pdi_send_msg.data[3] = (char)(cid & 0x00ff);
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		rc_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(rc_msg_buf, msg_len))
			return 1;
	}

	if (msg_len == 1) {
		if (msg_res.data[1] == 0x61)
			memcpy(data, (msg_res.data + 4), msg_res.data[0] - 3);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 5), 3);
		data += 3;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (rc_msg_buf + i)->data + 1, 7);
			data += 7;
		}
	}

	return 0;
}

/*checking JAMA*/
static int rc_check_JAMA(const struct pdi_cfg_s *sr)
{
	const struct pdi_rule_s* pdi_cfg_rule;
	int i;
	for(i = 0; i < sr->nr_of_rules; i ++) {
		pdi_cfg_rule = pdi_rule_get(sr, i);
		if (&pdi_cfg_rule == NULL) {
			printf("##START##EC-no JAMA rule...##END##\n");
			return 1;
		}

		switch(pdi_cfg_rule->type) {
		case PDI_RULE_JAMA:
			printf("##START##EC-Checking JAMA...##END##\n");
			if(JAMA_on())
				rc_data_buf[0] = 0x00;			//JAMA absent
			else rc_data_buf[0] = 0x01;			//JAMA present
			break;
		case PDI_RULE_UNDEF:
			return 1;
		}

		if(pdi_verify(pdi_cfg_rule, rc_data_buf) == 0)
			continue;
		else return 1;
	}

	return 0;
}

/*checking Part NO.*/
static int rc_check_partnr()
{
	int msg_len;
	char part_data[20];
	can_msg_t msg_res;
	printf("##START##EC-Checking part NO.##END##\n");
	if(usdt_GetDiagFirstFrame(&rc_getpart_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if(msg_len > 1) {
		rc_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(rc_msg_buf, msg_len))
			return 1;
	}
	/*pickup partnumber*/
	memcpy(part_data, (msg_res.data + 4), 4);
	memcpy((part_data + 4) ,(rc_msg_buf + 1)->data + 1 ,1);

	/*check partnumber*/
	if(memcmp(part_data, bcode_1 + 2, 3))
		return 1;
	if((part_data[3] != 0x30) || (part_data[4] != 0x41))
		return 1;

	return 0;
}

/*checking barcode*/
static int rc_check_barcode()
{
	char read_bcode[14];
	printf("##START##EC-Checking barcode...##END##\n");
	if(rc_GetCID(0x90ab, rc_data_buf))
		return 1;

	memcpy(read_bcode, rc_data_buf, 14);
	printf("##START##RB-");
	printf(read_bcode,"\0");
	printf("##END##\n");

	if (memcmp(rc_data_buf, bcode_1, 14))
		return 1;

	return 0;
}

/*clear error code*/
int pdi_clear_dtc(void)
{
	int msg_len;
	can_msg_t msg;

	pdi_mdelay(5000);
	pdi_startsession();
	if (usdt_GetDiagFirstFrame(&rc_clrdtc_msg, 1, NULL, &msg, &msg_len))
		return 1;
	if (msg.data[1] != 0x7b)	/*positive response is 0x7b*/
		return 1;
	return 0;
}

/*get fault from ECU,saved to data,the nr of fault is pnum_fault*/
static int rc_GetFault(char *data, int * pnum_fault)
{
	int i, result = 0;

	if(rc_GetCID(0x9080, data))
		return 1;

	memset(data + 117, 0x00, 10);

	for (i = 0; i < 117; i += 3) {
		if (data[i] | data[i+1] | data[i+2])
			result ++;
	}

	* pnum_fault = result;

	return 0;
}


int main(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 500000,
	};

	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[15], temp[2];
	int num_fault, rate;

	/*init all*/
	ulp_init();
	pdi_drv_Init();						//led,beep
	pdi_relay_init();
	ls1203_Init(&pdi_ls1203);			//scanner
	pdi_can_bus->init(&cfg_pdi_can);	//init can
	usdt_Init(pdi_can_bus);

	/*init action*/
	pre_check_action();

	/*check process*/
	while(1) {
		memset(bcode, 0, sizeof(bcode));
		while(ls1203_Read(&pdi_ls1203, bcode)) {
			ulp_update();
			if(target_on()) start_botton_on();
			else start_botton_off();
		}
		/*action start checking*/
		start_action();

		/*barcode processing*/
		bcode[14] = '\0';
		memcpy(bcode_1, bcode, 14);
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[5] = '\0';

		printf("##START##STATUS-5##END##\n");

		/*check ECU ready*/
		if(!target_on()) {
			pdi_noton_action();
			printf("##START##EC-Target is not on the right position##END##\n");
			continue;
		}

		/*get cfg file */
		pdi_cfg_file = pdi_cfg_get(bcode);

		/*whether or not the config file exist*/
		if(pdi_cfg_file == NULL) {
			pdi_fail_action();
			printf("##START##EC-No This Config File##END##\n");
			continue;
		}

		/*if ECU on,set relays*/
		pdi_set_relays(pdi_cfg_file);

		/*ECU power on*/
		pdi_IGN_on();

		/*delay 1s*/
		for (rate = 5; rate <= 17; rate ++) {
		pdi_mdelay(89);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
		}

		pdi_startsession();

		/*check barcode*/
		if(rc_check_barcode()) {
			pdi_fail_action();
			printf("##START##EC-Barcode Wrong##END##\n");
			continue;
		}
		printf("##START##EC-      Checking barcode Done...##END##\n");

		/*check JAMA*/
		if(rc_check_JAMA(pdi_cfg_file)) {
			pdi_fail_action();
			printf("##START##EC-JAMA wrong...##END##\n");
			continue;
		}
		printf("##START##EC-      Checking JAMA Done...##END##\n");

		/*check part NO.*/
		if(rc_check_partnr()) {
			pdi_fail_action();
			printf("##START##EC-PartNumber Wrong...##END##\n");
			continue;
		}
		printf("##START##EC-      Checking part NO. Done...##END##\n");

		printf("##START##EC-Waiting for ECU ready...##END##\n");
		/*delay 7s*/
		for (rate = 17; rate <= 96; rate ++) {
			pdi_mdelay(89);
			printf("##START##STATUS-");
			sprintf(temp, "%d", rate);
			printf("%s", temp);
			printf("##END##\n");
		}

		pdi_startsession();

		for(int i = 3; i > 0; i --) {
			int result = rc_GetFault(rc_fault_buf, &num_fault);
			if(result != 0 && i == 1) {
				printf("##START##STATUS-Get DTC error!##END##\n");
				pdi_fail_action();
				goto ERR_x;
			}
			if(result != 0)
				continue;
			if (num_fault) {
				printf("##START##EC-");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*3; i += 3)
					printf("0x%2x, 0x%2x, 0x%2x\n", rc_fault_buf[i]&0xff, rc_fault_buf[i+1]&0xff, rc_fault_buf[i+2]&0xff);
				printf("##END##\n");
				pdi_fail_action();
				goto ERR_x;
			} else break;
		}

		pdi_pass_action();

		ERR_x:
			printf("##START##STATUS-100##END##\n");
	}
}

#if 1
static int cmd_rc_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"rc , usage:\n"
		"rc fault		read the error code\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][0] == 'f') {
			pdi_IGN_on();
			pdi_mdelay(100);
			pdi_startsession();
			if (rc_GetFault(rc_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*3; i += 3)
					printf("0x%2x, 0x%2x, 0x%2x\n", rc_fault_buf[i]&0xff, rc_fault_buf[i+1]&0xff, rc_fault_buf[i+2]&0xff);
			}
			pdi_IGN_off();
		}
	}

	return 0;
}

const cmd_t cmd_rc = {"rc", cmd_rc_func, "rc cmd i/f"};
DECLARE_SHELL_CMD(cmd_rc)
#endif