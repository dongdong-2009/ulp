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
#include "ulp/debug.h"
#include "shell/cmd.h"
#include "led.h"

//local can msg varibles for all variants
static const can_msg_t sdm_wakeup_msg =		{0x100, 8, {0, 0, 0, 0, 0, 0, 0, 0}, 0};
static const can_msg_t sdm_exmode_msg = 	{0x247, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0};
static const can_msg_t sdm_cpid_msg = 		{0x247, 8, {0x03, 0xae, 0x01, 0x3f, 0, 0, 0, 0}, 0};
static const can_msg_t sdm_621_acc_msg =	{0x621, 8, {0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0};
static const can_msg_t sdm_621_crank_msg =	{0x621, 8, {0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0};
static const can_msg_t sdm_621_off_msg =	{0x621, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0};
static const can_msg_t sdm_testpresent_msg ={0x101, 3, {0xfe, 0x01, 0x3e}, 0};
// static const can_msg_t sdm_bcm_msg =		{0x13FFE040, 0, {0}, 1};
// static const can_msg_t sdm_ipc_msg =		{0x13FFE060, 0, {0}, 1};
static const can_msg_t sdm_misc_msg =		{0x103A6040, 2, {0, 0}, 1};

//local power mode can msg varibles for sdm10/sdm10+
static const can_msg_t sdm10_power_acc_msg =	{0x10242040, 1, {0x01}, 1};
static const can_msg_t sdm10_power_crank_msg =	{0x10242040, 1, {0x03}, 1};
static const can_msg_t sdm10_power_off_msg =	{0x10242040, 1, {0x00}, 1};
static const can_msg_t sdm10_pwrbp_crank_msg =	{0x10244060, 1, {0x03}, 1};
static const can_msg_t sdm10_pwrbp_off_msg =	{0x10244060, 1, {0x00}, 1};

//local power mode can msg varibles for sdm11/sdm11+
static const can_msg_t sdm11_power_acc_msg =	{0x10002040, 4, {0x01,0x00,0x00,0x00}, 1};
static const can_msg_t sdm11_power_crank_msg =	{0x10002040, 4, {0x03,0x00,0x00,0x00}, 1};
static const can_msg_t sdm11_power_off_msg =	{0x10002040, 4, {0x00,0x00,0x00,0x00}, 1};
static const can_msg_t sdm11_pwrbp_crank_msg =	{0x10004060, 1, {0x03}, 1};
static const can_msg_t sdm11_pwrbp_off_msg =	{0x10004060, 1, {0x00}, 1};

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

static struct can_queue_s sdm_present_msg = {
	.ms = 1000,
	.msg = {0x101, 3, {0xfe, 0x01, 0x3e}, 0},
};

static struct can_queue_s sdm_621_msg = {
	.ms = 1000,
	.msg = {0x621, 8, {0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s sdm_nca_bcm_msg = {
	.ms = 400,
	.msg = {0x13FFE040, 0, {0}, 1},
};

static struct can_queue_s sdm_nca_ipc_msg = {
	.ms = 400,
	.msg = {0x13FFE060, 0, {0}, 1},
};

static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char sdm_data_buf[256];
static char sdm_fault_buf[64];
static char bcode_1[20];
static int nca_periodic_flag = 0;
static int power_periodic_flag = 0;
static int sdm10plus_2435_flag = 0;

static LIST_HEAD(can_queue);

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

//local functions for can communication
static int sdm_GetDID(char did, char *data);
static int sdm_GetDPID(char dpid, char *data);
static int sdm_GetFault(char *data, int * pnum_fault);
static int sdm_clear_dtc(void);
static int sdm_wakeup(void);
static int sdm_sleep(void);

//local functions for pdi action
static int pdi_pass_action();
static int pdi_fail_action();
static int pdi_led_start();
static int target_noton_action();
static int counter_pass_add();
static int counter_fail_add();
static int sdm_init_OK();

//for test consequence
static int sdm_init();
static int sdm_check_init(const struct pdi_cfg_s *);
static int sdm_check(const struct pdi_cfg_s *);
static void sdm_process();

//others
static int sdm_mdelay(int );
static void sdm_InitMsg();
static void sdm_update();

static int sdm_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			ulp_update();
			sdm_update();
		}
	} while(left > 0);

	return 0;
}

static void sdm_InitMsg(void)
{
	INIT_LIST_HEAD(&sdm_present_msg.list);
	list_add(&sdm_present_msg.list, &can_queue);

	INIT_LIST_HEAD(&sdm_621_msg.list);
	list_add(&sdm_621_msg.list, &can_queue);

	INIT_LIST_HEAD(&sdm_nca_bcm_msg.list);
	list_add(&sdm_nca_bcm_msg.list, &can_queue);

	INIT_LIST_HEAD(&sdm_nca_ipc_msg.list);
	list_add(&sdm_nca_ipc_msg.list, &can_queue);
}

static void sdm_update(void)
{
	struct list_head *pos;
	struct can_queue_s *q;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			q -> timer = time_get(q -> ms);
			if (q -> msg.id == 0x621) {
				if (power_periodic_flag)
					pdi_can_bus -> send(&q -> msg);
			}
			if (q -> msg.id == 0x101) {
				if (power_periodic_flag)
					pdi_can_bus -> send(&q -> msg);
			}
			if (q -> msg.id == 0x13FFE040) {
				if (nca_periodic_flag)
					pdi_can_bus -> send(&q -> msg);
			}
			if (q -> msg.id == 0x13FFE060) {
				if (nca_periodic_flag)
					pdi_can_bus -> send(&q -> msg);
			}
		}
	}
}

static int sdm_init_OK()
{
	led_on(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	sdm_mdelay(200);
	led_off(LED_RED);
	led_off(LED_GREEN);
	beep_off();
	sdm_mdelay(100);
	for(int i = 0; i < 5; i++) {
		led_on(LED_RED);
		led_on(LED_GREEN);
		sdm_mdelay(200);
		led_off(LED_RED);
		led_off(LED_GREEN);
		sdm_mdelay(100);
	}
	return 0;
}

static int counter_pass_add()
{
	counter_pass_rise();
	sdm_mdelay(40);
	counter_pass_down();
	return 0;
}

static int counter_fail_add()
{
	counter_fail_rise();
	sdm_mdelay(40);
	counter_fail_down();
	return 0;
}

static int pdi_fail_action()
{
	sdm_sleep();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_RED);
	counter_fail_add();
	beep_on();
	sdm_mdelay(3000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	sdm_sleep();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	sdm_mdelay(20);
	printf("##START##EC-Test Result : No Error ##END##\n");
	counter_pass_add();
	sdm_mdelay(500);
	beep_off();
	sdm_mdelay(120);
	beep_on();
	sdm_mdelay(500);
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
		sdm_mdelay(200);
		beep_off();
		led_off(LED_GREEN);
		led_off(LED_RED);
		sdm_mdelay(100);
	}
	for(int i = 0; i < 4; i++) {
		led_on(LED_GREEN);
		led_on(LED_RED);
		sdm_mdelay(200);
		led_off(LED_GREEN);
		led_off(LED_RED);
		sdm_mdelay(100);
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

static int sdm_GetDID(char did, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x247, 8, {0x02, 0x1a, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;
	//char *temp = data;

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
		if (msg_res.data[1] == 0x5a)//说明读数据正确
			memcpy(data, (msg_res.data + 3), msg_res.data[0] - 2);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 4), 4);
		printf("%d\n", data);
		data += 4;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (pdi_msg_buf + i)->data + 1, 7);
			data += 7;
		}
		printf("%d\n",data);
	}

#if 0
	if (did == 0x72) {
		printf("DID MSG LEN: %d", msg_len);
		for (i = 0; i < 82; i++) {
			printf("0x%2x, ", temp[i]);
			if (i%10 == 0)
				printf("\n");
		}
		can_msg_print(&msg_res, "\n");
	}
#endif

	return 0;
}

static int sdm_GetDPID(char dpid, char *data)
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

static int check_barcode(void)
{
	int try_times = 5;

	printf("##START##EC-checking barcode##END##\n");

	while (sdm_GetDID(0xB4, sdm_data_buf)) {
		try_times --;
		if (try_times < 0)
			return 1;
	}
	sdm_data_buf[16] = '\0';
	printf("##START##RB-");
	printf(sdm_data_buf);
	printf("##END##\n");
	if (memcmp(bcode_1 + 3, sdm_data_buf, 16))
		return 1;
	else
		return 0;
}

static int sdm_clear_dtc()
{
	can_msg_t pdi_recv_msg;
	can_msg_t pdi_send_msg = {0x247, 8, {0x01, 0x04, 0, 0, 0, 0, 0, 0}, 0};
	time_t over_time;

	pdi_can_bus->send(&pdi_send_msg);
	over_time = time_get(100);
	do {
		if (time_left(over_time) < 0)
			return 1;
		if (pdi_can_bus->recv(&pdi_recv_msg) == 0) {
			if (pdi_recv_msg.data[1] == 0x44)
				break;
			else
				return 1;
		}
	} while(1);
	return 0;
}

static int sdm_GetFault(char *data, int * pnum_fault)
{
	int i = 0, result = 0;
	if (sdm_GetDID(0x71, data))
		return 1;

	memset(data + 90, 0x00, 38);

	for (i = 0; i < 45; i += 3) {
		if (data[i] | data[i+1] | data [i + 2])
			result ++;
	}

	* pnum_fault = result;

	return 0;
}

static int sdm_check_init(const struct pdi_cfg_s *sr)
{
	char *o=(char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	return 0;
}

static int sdm_check(const struct pdi_cfg_s *sr)
{
	int i, try_times = 5, rate;
	int num_fault;
	char temp[2];
	const struct pdi_rule_s* pdi_cfg_rule;

	if(sdm_wakeup())
		return 1;

	if(check_barcode()) {
		printf("##START##EC-Barcode Wrong##END##\n");
		return 1;
	}

	printf("##START##EC-ECU will be ready to read error buffer...##END##\n");
	for(rate = 19; rate < 95; rate ++) {
		sdm_mdelay(80);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	printf("##START##EC-checking limit file...##END##\n");

	for(i = 0; i < sr->nr_of_rules; i ++) {
		try_times = 5;
		pdi_cfg_rule = pdi_rule_get(sr, i);
		if (&pdi_cfg_rule == NULL) {
			printf("##START##EC-no this rule##END##\n");
			return 1;
		}

		switch(pdi_cfg_rule->type) {
		case PDI_RULE_DID:
			printf("##START##EC-checking DID:");
			printf("%X", (char *)pdi_cfg_rule->para);
			printf("##END##\n");
			while (sdm_GetDID(pdi_cfg_rule->para, sdm_data_buf)) {
				try_times --;
				if (try_times <= 0) {
					printf("##START##EC-read DID:");
					printf("%X", (char *)pdi_cfg_rule->para);
					printf(" error ##END##\n");
					return 1;
				}
			}
			break;
		case PDI_RULE_DPID:
			printf("##START##EC-checking DPID:");
			printf("%X", (char *)pdi_cfg_rule->para);
			printf("##END##\n");
			while (sdm_GetDPID(pdi_cfg_rule->para, sdm_data_buf)) {
				try_times --;
				if (try_times <= 0) {
					printf("##START##EC-read DPID:");
					printf("%X", (char *)pdi_cfg_rule->para);
					printf(" error ##END##\n");
					return 1;
				}
			}
			sdm_mdelay(10);
			break;
		case PDI_RULE_UNDEF:
			return 1;
		}

		if(pdi_verify(pdi_cfg_rule, sdm_data_buf) == 0) {
			continue;
		} else {
			printf("##START##EC-Error: Fault is : ");
			if (pdi_cfg_rule->type == PDI_RULE_DID)
				printf("DID $");
			else if (pdi_cfg_rule->type == PDI_RULE_DPID)
				printf("DPID $");
			else
				printf("UNDEF $");
			printf("%X",(char *)pdi_cfg_rule->para);
			printf("##END##\n");
			return 1;
		}
	}

	printf("##START##EC-checking limite file done...##END##\n");
	printf("##START##EC-checking error buffer...##END##\n");

	while (sdm_GetFault(sdm_fault_buf, &num_fault)) {
		try_times --;
		if (try_times < 0) {
			printf("##START##EC-read DTC error##END##\n");
			return 1;
		}
	}

	if (num_fault) {
		//sdm_clear_dtc();
		printf("##START##EC-");
		printf("num of fault is: %d * ", num_fault);
		for (i = 0; i < num_fault*3; i += 3)
			printf("0x%2x, 0x%2x, 0x%2x * ", sdm_fault_buf[i]&0xff, sdm_fault_buf[i+1]&0xff, sdm_fault_buf[i+2]&0xff);
		printf("##END##\n");
		return 1;
	}
	printf("##START##EC-checking error buffer done...##END##\n");
	sdm_GetDPID(0x21, sdm_data_buf);
	for(i = 0; i < 8; i++) {
		printf("%2x,", sdm_data_buf[i]&0xff);
	}
	return 0;
}

void sdm_init(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 33330,
	};

	pdi_drv_Init();//led,beep
	mbi5025_Init(&pdi_mbi5025);//SPI总线	移位寄存器
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);//scanner
	pdi_swcan_mode();//SW can
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
	sdm_InitMsg();
}

void sdm_process(void)
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

		//check sdm10+ 2435
		if (memcmp(bcode + 5, "2435", 4) == 0)
			sdm10plus_2435_flag = 1;

		pdi_cfg_file = pdi_cfg_get(bcode);

		if(target_on()) {
			if(pdi_cfg_file == NULL) {//是否有此配置文件
				pdi_fail_action();
				printf("##START##EC-No This Config File##END##\n");
			} else {
				sdm_check_init(pdi_cfg_file);//relay config
				if(sdm_check(pdi_cfg_file) == 0)
					pdi_pass_action();
				else
					pdi_fail_action();
			}
		} else {
			target_noton_action();
			printf("##START##EC-target is not on the right position##END##\n");
		}
	}
}

static int sdm_wakeup(void)
{
	int i, rate;
	char temp[2];

	//for display the time process of test
	printf("##START##EC-Initialize LAN  communication##END##\n");

	pdi_batt_on();
	/* for display, delay 400ms, wait for relay steady */
	for(rate = 0; rate < 10; rate ++) {
		sdm_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	//send wakeup msg
	pdi_can_bus->send(&sdm_wakeup_msg);
	pdi_can_bus->send(&sdm_wakeup_msg);
	/* for display, delay 100ms,*/
	printf("##START##EC-Send High Voltage Wakeup##END##\n");
	for (rate = 10; rate < 13; rate ++) {  //delay 100ms
		sdm_mdelay(30);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	/* for display */
	printf("##START##EC-Transmitting Power Mode##END##\n");
	//send power acc mode msg
	pdi_can_bus->send(&sdm10_power_acc_msg);
	pdi_can_bus->send(&sdm10_power_acc_msg);
	sdm_mdelay(50);
	pdi_can_bus->send(&sdm11_power_acc_msg);
	pdi_can_bus->send(&sdm11_power_acc_msg);
	sdm_mdelay(50);
	pdi_can_bus->send(&sdm_621_acc_msg);
	sdm_mdelay(50);


	//send power crank mode msg
	i = 2;
	do {
		pdi_can_bus->send(&sdm10_power_crank_msg);    //for sdm10 power mode
		pdi_can_bus->send(&sdm10_power_crank_msg);
		sdm_mdelay(50);
		pdi_can_bus->send(&sdm11_power_crank_msg);    //for sdm11 power mode
		pdi_can_bus->send(&sdm11_power_crank_msg);
		sdm_mdelay(50);
		pdi_can_bus->send(&sdm_misc_msg);
		if (sdm10plus_2435_flag)
			nca_periodic_flag = 0;
		else
			nca_periodic_flag = 1;
		sdm_mdelay(100);
		pdi_can_bus->send(&sdm10_pwrbp_crank_msg);  //sdm10 power mode backup
		pdi_can_bus->send(&sdm10_pwrbp_crank_msg);
		sdm_mdelay(100);
		pdi_can_bus->send(&sdm11_pwrbp_crank_msg);  //sdm11 power mode backup
		pdi_can_bus->send(&sdm11_pwrbp_crank_msg);
		sdm_mdelay(100);
		pdi_can_bus->send(&sdm_621_crank_msg);
		pdi_can_bus->send(&sdm_621_crank_msg);
		sdm_mdelay(100);
	} while(i -- );

	power_periodic_flag = 1;                       //enable power msg periodic flag

	//start the extend diagnostic session and send cpid msg
	pdi_can_bus->send(&sdm_exmode_msg);
	sdm_mdelay(20);
	pdi_can_bus->send(&sdm_cpid_msg);
	sdm_mdelay(20);
	pdi_can_bus->send(&sdm_testpresent_msg);
	sdm_mdelay(20);

	/* for display*/
	printf("##START##STATUS-18##END##\n");

#if 0
	time_t deadline;
	can_msg_t msg;
	can_msg_t pdi_send_msg = {0x247, 8, {0x03, 0xaa, 0x01, 0x25, 0, 0, 0, 0}, 0};
	//for check, reading dpid
	for (i = 0; i < 30; i++) {
		pdi_can_bus->send(&pdi_send_msg);
		can_msg_print(&pdi_send_msg, "\n");
		deadline = time_get(100);
		do {
			if (pdi_can_bus->recv(&msg) == 0) {
				if (msg.id == 0x547) {
					can_msg_print(&msg, "\n\n");
					break;
				}
			}
		} while(time_left(deadline) > 0);
		sdm_mdelay(500);
	}
#endif

	return 0;
}

static int sdm_sleep(void)
{
	char temp[3];
	int rate;

	//turn off periodic flag
	nca_periodic_flag = 0;
	power_periodic_flag = 0;
	sdm10plus_2435_flag = 0;

	/* for display*/
	printf("##START##EC---------------------------------------##END##\n");
	printf("##START##EC-Transmitting Power Mode Off##END##\n");

	//send power off mode msg
	pdi_can_bus->send(&sdm10_power_off_msg);
	pdi_can_bus->send(&sdm10_power_off_msg);
	sdm_mdelay(40);
	pdi_can_bus->send(&sdm11_power_off_msg);
	pdi_can_bus->send(&sdm11_power_off_msg);
	sdm_mdelay(40);
	pdi_can_bus->send(&sdm10_pwrbp_off_msg);
	pdi_can_bus->send(&sdm10_pwrbp_off_msg);
	sdm_mdelay(40);
	pdi_can_bus->send(&sdm11_pwrbp_off_msg);
	pdi_can_bus->send(&sdm11_pwrbp_off_msg);
	sdm_mdelay(40);
	pdi_can_bus->send(&sdm_621_off_msg);
	pdi_can_bus->send(&sdm_621_off_msg);

	/* for display*/
	for (rate = 95; rate <= 100; rate ++) {
		sdm_mdelay(50);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	pdi_batt_off();

	return 0;
}

int main(void)
{
	ulp_init();
	sdm_init();
	sdm_init_OK();
	while(1) {
		sdm_process();
		ulp_update();
		sdm_update();
	}
}

#if 1
static int cmd_sdm_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"sdm , usage:\n"
		"sdm fault\n"
		"sdm clear\n"
		"sdm wakeup, it maybe result in some error code!\n"
		"sdm sleep,  it maybe result in some error code!\n"
		"sdm batt on/off\n"
		"sdm IGN on/off\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][0] == 'f') {
			if (sdm_GetFault(sdm_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*3; i += 3)
					printf("0x%2x, 0x%2x, 0x%2x\n", sdm_fault_buf[i]&0xff, sdm_fault_buf[i+1]&0xff, sdm_fault_buf[i+2]&0xff);
			}
		}
		if(argv[1][0] == 'c') {
			sdm_wakeup();
			sdm_mdelay(2000);
			sdm_clear_dtc();
			sdm_mdelay(2000);
			sdm_sleep();
			printf("##OK##\n");
		}
		if(argv[1][0] == 'w') {
			sdm_wakeup();
		}
		if(argv[1][0] == 's') {
			sdm_sleep();
		}
	}
	if(argc == 3) {
		if(argv[1][0] == 'b') {
			if(argv[2][1] == 'n')
				pdi_batt_on();
			if(argv[2][1] == 'f')
				pdi_batt_off();
		}
		if(argv[1][0] == 'I') {
			if(argv[2][1] == 'n')
				pdi_IGN_on();
			if(argv[2][1] == 'f')
				pdi_IGN_off();
		}
	}

	return 0;
}

const cmd_t cmd_sdm = {"sdm", cmd_sdm_func, "sdm cmd i/f"};
DECLARE_SHELL_CMD(cmd_sdm)
#endif
