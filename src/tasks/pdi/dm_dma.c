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

#define PDI_DEBUG	1

static const can_msg_t dm_rddtc_msg = {
	0x607, 8, {0x03, 0x19, 0x89, 0x08, 0, 0, 0, 0}, 0
};
static const can_msg_t dm_clrdtc_msg = {
	0x607, 8, {0x04, 0x14, 0xff, 0xff, 0xff, 0, 0, 0}, 0
};
static const can_msg_t dm_bcode_msg = {
	0x607, 8, {0x03, 0x22, 0xFE, 0x8D, 0, 0, 0, 0}, 0
};
static const can_msg_t dm_errcode_msg = {
	0x607, 8, {0x03, 0x22, 0xfe, 0x80, 0, 0, 0, 0}, 0
};
static const can_msg_t dm_getsn_msg = {
	0x607, 8, {0x03, 0x22, 0xfe, 0x8d, 0, 0, 0, 0}, 0
};
static const can_msg_t dm_reqseed_msg = {
	0x607, 8, {0x02, 0x27, 0x7d, 0, 0, 0, 0, 0}, 0
};
static const can_msg_t dm_start_msg = {
	0x607, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0
};

static const mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};

static const ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 19,//长度没有定下来
		.dead_time = 20,
};

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

static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char pdi_data_buf[256];			//data buffer
static char pdi_fault_buf[64];			//fault buffer
static char bcode_1[19];

static int pdi_check();
static int pdi_GetFault(char *data, int * pnum_fault);
static int pdi_clear_dtc();
static int pdi_check_init(const struct pdi_cfg_s *);
static int check_barcode();
static void pdi_process();
static int pdi_pass_action();
static int pdi_fail_action();
static int target_noton_action();
static int pdi_led_start();
static int dm_mdelay(int );
static int init_OK();
static int counter_pass_add();
static int counter_fail_add();
static void dm_update();
static void dm_InitMsg();
static int dm_StartSession();
static int dm_GetCID(short cid, char *data);
static int esc_check();

/**************************************************************************/
/************         Local funcitons                         *************/
/**************************************************************************/
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

//for start the session
static int dm_StartSession(void)
{
	int i, msg_len;
	unsigned char seed[2], result;
	can_msg_t msg;
	can_msg_t sendkey_msg = {
		0x607,
		8,
		{0x04, 0x27, 0x7e, 0xff, 0xff, 0, 0, 0},
		0
	};

	if (usdt_GetDiagFirstFrame(&dm_start_msg, 1, NULL, &msg, &msg_len))   //start session
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif
	if (usdt_GetDiagFirstFrame(&dm_reqseed_msg, 1, NULL, &msg, &msg_len)) //req seed
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

	//calculate the key from seed
	seed[0] = (unsigned char)msg.data[3];
	seed[1] = (unsigned char)msg.data[4];
	result = seed[0] ^ seed[1];
	result ^= 0x23;
	result ^= 0x9a;
	sendkey_msg.data[3] = (char)((0x239a + result) >> 8);
	sendkey_msg.data[4] = (char)((0x239a + result) & 0x00ff);

	if (usdt_GetDiagFirstFrame(&sendkey_msg, 1, NULL, &msg, &msg_len)) //send key
		return 1;
	//judge the send key response
	if ((msg.data[1] != 0x67) || (msg.data[2] != 0x7e))
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

#if 0
	//get serial number
	printf("\nSN Code:\n");
	usdt_GetDiagFirstFrame(&dm_getsn_msg, 1, NULL, pdi_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(pdi_msg_buf + i, "\n");

	// get error code
	printf("\nError Code:\n");
	usdt_GetDiagFirstFrame(&dm_errcode_msg, 1, NULL, pdi_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(pdi_msg_buf + i, "\n");

	// get dtc code
	printf("\nDTC Code:\n");
	usdt_GetDiagFirstFrame(&dm_rddtc_msg, 1, NULL, pdi_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(pdi_msg_buf + i, "\n");
#endif

	return 0;
}

static int dm_GetCID(short cid, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x607, 8, {0x03, 0x22, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;
	//char *temp = data;

	pdi_send_msg.data[2] = (char)(cid >> 8);
	pdi_send_msg.data[3] = (char)(cid & 0x00ff);
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		pdi_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len))
			return 1;
	}

	//pick up the data
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x62)
			memcpy(data, (msg_res.data + 4), msg_res.data[0] - 3);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 5), 3);
		data += 3;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (pdi_msg_buf + i)->data + 1, 7);
			data += 7;
		}
	}

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
			pdi_can_bus -> send(&q -> msg);
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

static int esc_check()
{

}

static int check_barcode()
{
	//start session
	if (dm_StartSession())
		return 1;

	dm_GetCID(0xfe8d, pdi_data_buf);

	if (memcmp(pdi_data_buf, bcode_1, 19))
		return 1;
	else
		return 0;
}

static int pdi_clear_dtc(void)
{
	int msg_len;
	can_msg_t msg;

	//start session
	if (dm_StartSession())
		return 1;

	if (usdt_GetDiagFirstFrame(&dm_clrdtc_msg, 1, NULL, &msg, &msg_len))
		return 1;
	if (msg.data[1] != 0x54)	//positive response is 0x54
		return 1;

	return 0;
}

static int pdi_GetFault(char *data, int * pnum_fault)
{
	//start session
	if (dm_StartSession())
		return 1;


	return 0;
}

static int pdi_check()
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

#if 1
static int cmd_dm_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"dm , usage:\n"
		"dm fault\n"
		"dm clear\n"
		"dm batt on/off\n"
		"dm start, start the diagnostic seesion\n"
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
			if (pdi_clear_dtc())
				printf("##ERROR##\n");
			else
				printf("##OK##\n");
		}

		// start the diagnostic session
		if(argv[1][0] == 's') {
			dm_StartSession();
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
#endif
