/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add content for PDI_SDM10
 */
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "drv.h"
#include "mbi5025.h"
#include "ls1203.h"
#include "cfg.h"
#include "can.h"

//static time_t pdi_loop_timer;
const can_bus_t* pdi_can_bus = &can1;

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

static int pdi_mdelay(int ms)
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
	counter_total_add();
	beep_on();
	pdi_mdelay(3000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	led_pass_on();
	beep_on();
	counter_total_add();
	pdi_mdelay(1000);
	beep_off();
	return 0;
}

static int pdi_check(const struct pdi_cfg_s *sr)
{
	const struct pdi_rule_s* pdi_cfg_rule;
	can_msg_t pdi_send_msg;
	can_msg_t pdi_recv_msg;
	//relay
	power_on();
	for(int i = 0;i < sr->nr_of_rules;i++) {
		pdi_cfg_rule = pdi_rule_get(sr,i);
		if(&pdi_cfg_rule == NULL) {
			return 1;
		}
		switch(pdi_cfg_rule->type) {
		case PDI_RULE_DID:
			pdi_send_msg = { //
				.id = 0x247,
				.dlc = 3,
				.data = {0x02,0x1A,pdi_cfg_rule->para}
			};
		case PDI_RULE_DPID:
			pdi_send_msg = { //
				.id = 0x247,
				.dlc = 4,
				.data = {0x03,0xAA,0x01,pdi_cfg_rule->para}
			};
		case PDI_RULE_UNDEF:
			return 1;
		}
		pdi_can_bus->send(&pdi_send_msg);
		time_t over_time = time_get(50);
		while(over_time)
			if(pdi_can_bus->recv(&pdi_recv_msg))
				if(pdi_verify(pdi_cfg_rule,pdi_recv_msg.data) == 0)
					i++;
				else return 1;
			else return 1;
	}
	return 0;
}

static void pdi_init(void)
{
	pin_init();
	can_cfg_t pdi_can = {
		.baud = 33330,
	};
	mbi5025_Init(&pdi_mbi5025);
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);
	pdi_swcan_mode();
	pdi_can_bus->init(&pdi_can);
}

static void pdi_update(void)
{
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[19];
	if(check_start() == 1) {
		while(1) {
			if(ls1203_Read(&pdi_ls1203,bcode) == 0) {
				bcode[9] = '\0';
				led_fail_off();
				led_pass_off();
				if(target_on() == 1) {
					pdi_cfg_file = pdi_cfg_get(bcode);
					if(pdi_check(pdi_cfg_file) == 0)
						pdi_pass_action();
				}
				else {
					pdi_fail_action();
					continue;
				}
			}
		}
	}
}

int main(void)
{
	ulp_init();
	pdi_init();
	while(1) {
#if 0
		printf("loop = %dmS\n", - time_left(pdi_loop_timer));
		pdi_loop_timer = time_get(0);
#endif
		pdi_update();
	}
}
