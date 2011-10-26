/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add content for PDI_SDM10
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

//local varibles;
static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char pdi_data_buf[32];

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
	led_fail_on();
	counter_fail_add();
	beep_on();
	pdi_mdelay(3000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	led_pass_on();
	beep_on();
	counter_pass_add();
	pdi_mdelay(1000);
	beep_off();
	return 0;
}

static int pdi_GetDID(char did, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x247, 8, {0x02, 0x1a, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = did;
	msg_len = usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res);
	if (msg_len > 1) {
		pdi_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len))
			return 1;
	}

	//pick up the data
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x5a) {
			memcpy(data, (msg_res.data + 3), msg_res.data[0] - 2);
			return 0;
		} else {
			return 1;
		}
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 4), 4);
		data += 4;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (pdi_msg_buf + i)->data + 1, 7);
			data += 7;
		}
		return 0;
	} else
		return -1;
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
	int i;
	const struct pdi_rule_s* pdi_cfg_rule;


	mbi5025_WriteBytes(&pdi_mbi5025, (unsigned char*)sr->relay, 4);
	power_on();
	pdi_mdelay(100);
	led_fail_off();
	led_pass_off();
	for(i = 0; i < sr->nr_of_rules; i++) {
		pdi_cfg_rule = pdi_rule_get(sr,i);
		if (&pdi_cfg_rule == NULL)
			return 1;
		switch(pdi_cfg_rule->type) {
		case PDI_RULE_DID:
			pdi_GetDID(pdi_cfg_rule->para, pdi_data_buf);
			break;
		case PDI_RULE_DPID:
			pdi_GetDPID(pdi_cfg_rule->para, pdi_data_buf);
			break;
		case PDI_RULE_UNDEF:
			return 1;
		}
		if(pdi_verify(pdi_cfg_rule, pdi_data_buf) == 0)
			continue;
		else
			return 1;
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
		bcode[9] = '\0';
		pdi_cfg_file = pdi_cfg_get(bcode);
		if(pdi_check(pdi_cfg_file) == 0)
			pdi_pass_action();
		else
			pdi_fail_action();
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
