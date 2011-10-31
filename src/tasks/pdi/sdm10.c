/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add contents for PDI_SDM10
 */
#include <string.h>
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "can.h"
#include "drv.h"
#include "Mbi5025.h"
#include "ls1203.h"
#include "cfg.h"
#include "priv/usdt.h"
#include "debug.h"
#include "shell/cmd.h"

//local varibles;
static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char pdi_data_buf[32];
static char pdi_fault_buf[128];

static mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};

static ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 19,
		.dead_time = 20,
};

//local functions
static int pdi_check(const struct pdi_cfg_s *);
static int pdi_mdelay(int );
static int pdi_pass_action();
static int pdi_fail_action();
static int pdi_GetDID(char did, char *data);
static int pdi_GetDPID(char dpid, char *data);
static int pdi_check(const struct pdi_cfg_s *sr);
static int pdi_GetFault(char *data, int * pnum_fault);
static int target_noton_action();

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

static int pdi_fail_action()
{
	led_pass_off();
	led_fail_on();
	counter_fail_add();
	beep_on();
	pdi_mdelay(1000);
	beep_off();
	pdi_mdelay(200);
	beep_on();
	pdi_mdelay(1000);
	beep_off();
	power_off();
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	start_botton_off();
	return 0;
}

static int pdi_pass_action()
{
	char temp[3];
	for(int j = 90; j <= 100; j++) {
		pdi_mdelay(50);
		printf("##START##STATUS-");
		sprintf(temp,"%d",j);
		printf("%s",temp);
		printf("##END##\n");
	}
	led_fail_off();
	led_pass_on();
	beep_on();
	counter_pass_add();
	pdi_mdelay(100);
	beep_off();
	power_off();
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	start_botton_off();
	return 0;
}

static int target_noton_action()
{
	led_pass_off();
	led_fail_on();
	beep_on();
	pdi_mdelay(200);
	beep_off();
	pdi_mdelay(100);
	beep_on();
	pdi_mdelay(200);
	beep_off();
	pdi_mdelay(100);
	beep_on();
	pdi_mdelay(200);
	beep_off();
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	mbi5025_WriteByte(&pdi_mbi5025, 0x00);
	start_botton_off();
	return 0;
}

static int pdi_GetDID(char did, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x247, 8, {0x02, 0x1a, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = did;
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		pdi_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len))
			return 1;
	}

	//pick up the data
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x5a)
			memcpy(data, (msg_res.data + 3), msg_res.data[0] - 2);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 4), 4);
		data += 4;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (pdi_msg_buf + i)->data + 1, 7);
			data += 7;
		}
	}

	return 0;
}

static int pdi_GetFault(char *data, int * pnum_fault)
{
	int i = 0, result = 0;
	if (pdi_GetDID(0x71, data))
		return 1;

	memset(data + 90, 0x00, 38);

	for (i = 0; i < 90; i += 3) {
		if (data[i] | data[i+1] | data [i + 2])
			result ++;
	}

	* pnum_fault = result;

	return 0;
}



static int pdi_GetDPID(char dpid, char *data)
{
	can_msg_t pdi_recv_msg;
	can_msg_t pdi_send_msg = {0x247, 8, {0x03, 0xaa, 0x01, 0, 0, 0, 0, 0}, 0};
	time_t over_time;

	pdi_send_msg.data[3] = dpid;
	pdi_can_bus->send(&pdi_send_msg);
	over_time = time_get(100);
	do {
		if (time_left(over_time) < 0)
			return 1;
		if (pdi_can_bus->recv(&pdi_recv_msg) == 0) {
			if (pdi_recv_msg.data[0] == dpid)
				break;
			else
				return 1;
		}
	} while(1);

	memcpy(data, (pdi_recv_msg.data + 1), 7);
	return 0;
}

static int pdi_check(const struct pdi_cfg_s *sr)
{
	int i, try_times;
	char temp[2];
	const struct pdi_rule_s* pdi_cfg_rule;
	char *o=(char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	power_on();
//	pdi_mdelay(6000);
	for(int k = 0; k < 90; k++) {
		pdi_mdelay(70);
		printf("##START##STATUS-");
		sprintf(temp,"%d",k);
		printf("%s",temp);
		printf("##END##\n");
	}
	led_fail_off();
	led_pass_off();
	for(i = 0; i < sr->nr_of_rules; i++) {
		try_times = 5;
		pdi_cfg_rule = pdi_rule_get(sr,i);
		if (&pdi_cfg_rule == NULL) {
			printf("##START##EC-no this rule##END##\n");
			return 1;
		}
		switch(pdi_cfg_rule->type) {
		case PDI_RULE_DID:
			while (pdi_GetDID(pdi_cfg_rule->para, pdi_data_buf)) {
				try_times --;
				if (try_times <= 0)
					return 1;
			}
			break;
		case PDI_RULE_DPID:
			while (pdi_GetDPID(pdi_cfg_rule->para, pdi_data_buf)) {
				try_times --;
				if (try_times <= 0)
					return 1;
			}
			break;
		case PDI_RULE_UNDEF:
			return 1;
		}
		if(pdi_verify(pdi_cfg_rule, pdi_data_buf) == 0) {
			if(i == 0) {
				printf("##START##RB-");
				printf(pdi_data_buf);
				printf("##END##\n");
			}
			continue;
		} else {
			printf("##START##EC-");
			if (pdi_cfg_rule->type == PDI_RULE_DID)
				printf("DID: $");
			else if (pdi_cfg_rule->type == PDI_RULE_DPID)
				printf("DPID: $");
			else
				printf("UNDEF: $");
			printf("%X",(char *)pdi_cfg_rule->para);
			printf("##END##\n");
			return 1;
		}
	}
	return 0;
}

void pdi_init(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 33330,
	};

	pdi_drv_Init();
	mbi5025_Init(&pdi_mbi5025);
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);
	pdi_swcan_mode();
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
}

void pdi_update(void)
{
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[19];
	if(ls1203_Read(&pdi_ls1203,bcode) == 0) {
		start_botton_on();
		led_pass_off();
		led_fail_off();
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[9] = '\0';
		pdi_cfg_file = pdi_cfg_get(bcode);
		if(pdi_cfg_file == NULL) {
			pdi_fail_action();
			printf("##START##EC-no this config file##END##\n");
		} else {
			if(target_on() == 1) {
				if(pdi_check(pdi_cfg_file) == 0)
					pdi_pass_action();
				else
					pdi_fail_action();
			} else {
				target_noton_action();
				printf("##START##EC-target is not on right position##END##\n");
			}
		}
	}
}

int main(void)
{
	ulp_init();
	pdi_init();
	while(1) {
		pdi_update();
		ulp_update();
	}
}

static int cmd_pdi_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"pdi power control, usage:\n"
		"pdi on\n"
		"pdi off\n"
		"pdi fault"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][1] == 'n') {
			power_on();
		}
		if(argv[1][1] == 'f') {
			power_off();
		}
		if(argv[1][0] == 'f') {
			if (pdi_GetFault(pdi_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				for (i = 0; i < num_fault; i++)
					printf("0x%x, 0x%x, 0x%x\n", pdi_fault_buf[i], pdi_fault_buf[i+1], pdi_fault_buf[i+2]);
			}
		}
	}

	return 0;
}

const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi cmd i/f"};
DECLARE_SHELL_CMD(cmd_pdi)