/*
 *	David peng.guo@2011 initial version
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
#include "ulp/debug.h"
#include "shell/cmd.h"
#include "led.h"
#include "pdi.h"

const can_msg_t dm_bcode_msg =		{0x607, 4, {0x03, 0x22, 0xFE, 0x8D}, 0};
const can_msg_t dm_DTC_msg =		{0x607, 4, {0x03, 0x22, 0xFE, 0x80}, 0};
const can_msg_t dm_reqseed_msg =	{0x607, 3, {0x02, 0x27, 0x01}, 0};
static const can_bus_t* pdi_can_bus = &can1;
static char pdi_fault_buf[64];
static char bcode_1[19];

static mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};

static ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 19,//长度没有定下来
		.dead_time = 20,
};

static int pdi_check(const struct pdi_cfg_s *);
static int pdi_GetFault(char *data, int * pnum_fault);
static int pdi_clear_dtc();
static int pdi_check_init(const struct pdi_cfg_s *);
static void pdi_process();
static int pdi_pass_action();
static int pdi_fail_action();
static int target_noton_action();
static int pdi_led_start();
static int dm_mdelay(int );
static int init_OK();
static int counter_pass_add();
static int counter_fail_add();
static int getbarcode();
static void dm_update();
static void dm_InitMsg();

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

static struct can_queue_s dm_msg1 = {
	.ms = 1000,
	.msg = {0x5FA, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s dm_msg2 = {
	.ms = 1000,
	.msg = {0x5FC, 5, {0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s dm_msg3 = {
	.ms = 40,
	.msg = {0x2F0, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s dm_msg4 = {
	.ms = 500,
	.msg = {0x607, 8, {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static LIST_HEAD(can_queue);
static const can_bus_t *can_bus = &can1;

static int dm_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			ulp_update();
			dm_update();
		}
	} while(left > 0);

	return 0;
}

static int init_OK()
{
	led_on(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	dm_mdelay(200);
	led_off(LED_RED);
	led_off(LED_GREEN);
	beep_off();
	dm_mdelay(100);
	for(int i = 0; i < 5; i++){
		led_on(LED_RED);
		led_on(LED_GREEN);
		dm_mdelay(200);
		led_off(LED_RED);
		led_off(LED_GREEN);
		dm_mdelay(100);
	}
	return 0;
}

static int counter_pass_add()
{
	counter_pass_rise();
	dm_mdelay(40);
	counter_pass_down();
	return 0;
}

static int counter_fail_add()
{
	counter_fail_rise();
	dm_mdelay(40);
	counter_fail_down();
	return 0;
}

static int pdi_fail_action()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_RED);
	counter_fail_add();
	beep_on();
	dm_mdelay(3000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	dm_mdelay(20);
	printf("##START##EC-Test Result : No Error ##END##\n");
	counter_pass_add();
	dm_mdelay(1000);
	beep_off();
	dm_mdelay(200);
	beep_on();
	dm_mdelay(1000);
	beep_off();
	return 0;
}

static int target_noton_action()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	for(int i = 0; i < 4; i++) {
		beep_on();
		led_on(LED_GREEN);
		led_on(LED_RED);
		dm_mdelay(200);
		beep_off();
		led_off(LED_GREEN);
		led_off(LED_RED);
		dm_mdelay(100);
	}
	for(int i = 0; i < 4; i++) {
		led_on(LED_GREEN);
		led_on(LED_RED);
		dm_mdelay(200);
		led_off(LED_GREEN);
		led_off(LED_RED);
		dm_mdelay(100);
	}
	return 0;
}

static int pdi_led_start()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_Update_Immediate();
	led_flash(LED_GREEN);
	led_flash(LED_RED);
	return 0;
}

void pdi_init(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 500000,
	};

	pdi_drv_Init();//led,beep
	mbi5025_Init(&pdi_mbi5025);//SPI总线	移位寄存器
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);//scanner
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
	dm_InitMsg();
}

static void dm_InitMsg(void)
{
	INIT_LIST_HEAD(&dm_msg1.list);
	list_add(&dm_msg1.list, &can_queue);

	INIT_LIST_HEAD(&dm_msg2.list);
	list_add(&dm_msg2.list, &can_queue);

	INIT_LIST_HEAD(&dm_msg3.list);
	list_add(&dm_msg3.list, &can_queue);

	INIT_LIST_HEAD(&dm_msg4.list);
	list_add(&dm_msg4.list, &can_queue);
}

static void dm_update()
{
	struct list_head *pos;
	struct can_queue_s *q;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			q -> timer = time_get(q -> ms);
			can_bus -> send(&q -> msg);
		}
	}
}

static int pdi_check_init(const struct pdi_cfg_s *sr)
{
	char *o = (char *)&(sr -> relay_ex);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	char *p = (char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(p+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+0));
	return 0;
}

static void pdi_process(void)
{
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[20];
	if(target_on())
		start_botton_on();
	else start_botton_off();
	if(ls1203_Read(&pdi_ls1203, bcode) == 0) {

		start_botton_off();
		pdi_led_start();
		bcode[19] = '\0';

		memcpy(bcode_1, bcode, 19);
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[9] = '\0';

		pdi_cfg_file = pdi_cfg_get(bcode);

		if(target_on()) {
			if(pdi_cfg_file == NULL) {//是否有此配置文件
				pdi_fail_action();
				printf("##START##EC-No This Config File##END##\n");
			}
			pdi_check_init(pdi_cfg_file);//relay config
			if(pdi_check(pdi_cfg_file) == 0)
				pdi_pass_action();
			else
				pdi_fail_action();
		} else {
			target_noton_action();
			printf("##START##EC-target is not on the right position##END##\n");
		}
	}
}

static int getbarcode(char *data)
{
	can_msg_t dm_recv_msg;
	can_msg_t dm_send_msg = {0x607, 8, {0x03, 0x22, 0xFE, 0x8D, 0x00, 0x00, 0x00, 0x00}, 0};


	return 0;
}

static int check_barcode()
{

}

static int pdi_clear_dtc(void)
{
	return 0;
}

static int pdi_GetFault(char *data, int * pnum_fault)
{
	return 0;
}

static int pdi_check(const struct pdi_cfg_s *sr)
{
	int i, num_fault, try_times = 5;
	const struct pdi_rule_s* pdi_cfg_rule;

	if(check_barcode()) {
		printf("##START##EC-Barcode Wrong##END##\n");
		return 1;
	}

	while (pdi_GetFault(pdi_fault_buf, &num_fault)) {
		try_times --;
		if (try_times < 0) {
			printf("##START##EC-read DTC error##END##\n");
			return 1;
		}
	}

	if (num_fault) {
		//pdi_clear_dtc();
		printf("##START##EC-");
		printf("num of fault is: %d*", num_fault);
		for (i = 0; i < num_fault*3; i += 3)
			printf("0x%2x, 0x%2x, 0x%2x*", pdi_fault_buf[i]&0xff, pdi_fault_buf[i+1]&0xff, pdi_fault_buf[i+2]&0xff);
		printf("##END##\n");
		return 1;
	}

	return 0;
}

int main(void)
{
	ulp_init();
	pdi_init();
	init_OK();
	while(1) {
		pdi_process();
		ulp_update();
		dm_update();
	}
}

static int cmd_dm_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"dm , usage:\n"
		"dm fault\n"
		"dm clear\n"
		"dm batt on/off\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][0] == 'f') {
			if (pdi_GetFault(pdi_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*3; i += 3)
					printf("0x%2x, 0x%2x, 0x%2x\n", pdi_fault_buf[i]&0xff, pdi_fault_buf[i+1]&0xff, pdi_fault_buf[i+2]&0xff);
			}
		}
		if(argv[1][0] == 'c') {
			pdi_clear_dtc();
			printf("##OK##\n");
		}
	}
	if(argc == 3) {
		if(argv[1][0] == 'b') {
			if(argv[2][1] == 'n')
				pdi_IGN_on();
			if(argv[2][1] == 'f')
				pdi_IGN_off();
		}
	}

	return 0;
}

const cmd_t cmd_dm = {"dm", cmd_dm_func, "dm cmd i/f"};
DECLARE_SHELL_CMD(cmd_dm)