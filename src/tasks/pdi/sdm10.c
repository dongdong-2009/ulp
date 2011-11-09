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
#include "led.h"

//local varibles;
const can_msg_t pdi_wakeup_msg =	{0x100, 8, {0, 0, 0, 0, 0, 0, 0, 0}, 0};
const can_msg_t pdi_dtc_msg = 		{0x247, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0};
const can_msg_t pdi_cpid_msg = 		{0x247, 8, {0x03, 0xae, 0x01, 0x3f, 0, 0, 0, 0}, 0};
const can_msg_t pdi_present_msg =	{0x101, 3, {0xfe, 0x01, 0x3e}, 0};
const can_msg_t pdi_621_msg =		{0x621, 8, {0x01, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0};
const can_msg_t pdi_10_power_cfg_msg =	{0x10242040, 1, {0x01}, 1};
const can_msg_t pdi_11_power_cfg_msg =	{0x10002040, 1, {0x01}, 1};
const can_msg_t pdi_10_pwr_on_msg1 =	{0x10242040, 1, {0x03}, 1};
const can_msg_t pdi_11_pwr_on_msg1 =	{0x10002040, 4, {0x03,0x00,0x00,0x00}, 1};
const can_msg_t pdi_10_pwr_on_msg2 =	{0x10244060, 1, {0x03}, 1};
const can_msg_t pdi_11_pwr_on_msg2 =	{0x10004060, 1, {0x03}, 1};
const can_msg_t pdi_10_pwr_off_msg1 =	{0x10242040, 1, {0x00}, 1};
const can_msg_t pdi_10_pwr_off_msg2 =	{0x10244060, 1, {0x00}, 1};
const can_msg_t pdi_11_pwr_off_msg1 =	{0x10002040, 4, {0x00,0x00,0x00,0x00}, 1};
const can_msg_t pdi_11_pwr_off_msg2 =	{0x10004060, 1, {0x00}, 1};
static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char pdi_data_buf[256];
static char pdi_fault_buf[64];
static char bcode_1[20];

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
static int pdi_pass_action();
static int pdi_fail_action();
static int pdi_GetDID(char did, char *data);
static int pdi_GetDPID(char dpid, char *data);
static int pdi_GetFault(char *data, int * pnum_fault);
static int target_noton_action();
static int pdi_clear_dtc();
static int pdi_wakeup();
static int pdi_sleep();
static int pdi_check_init(const struct pdi_cfg_s *);
static int pdi_led_start();
static int pdi_mdelay(int );
static int init_OK();
static int counter_pass_add();
static int counter_fail_add();
static void pdi_process();

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

static int init_OK()
{
	led_on(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	pdi_mdelay(200);
	led_off(LED_RED);
	led_off(LED_GREEN);
	beep_off();
	pdi_mdelay(100);
	for(int i = 0; i < 5; i++){
		led_on(LED_RED);
		led_on(LED_GREEN);
		pdi_mdelay(200);
		led_off(LED_RED);
		led_off(LED_GREEN);
		pdi_mdelay(100);
	}
	return 0;
}

static int counter_pass_add()
{
	counter_pass_rise();
	pdi_mdelay(40);
	counter_pass_down();
	return 0;
}

static int counter_fail_add()
{
	counter_fail_rise();
	pdi_mdelay(40);
	counter_fail_down();
	return 0;
}

static int pdi_fail_action()
{
	pdi_sleep();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_RED);
	counter_fail_add();
	beep_on();
	pdi_mdelay(3000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	pdi_sleep();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	pdi_mdelay(20);
	printf("##START##EC-Test Result : No Error ##END##\n");
	counter_pass_add();
	pdi_mdelay(1000);
	beep_off();
	pdi_mdelay(200);
	beep_on();
	pdi_mdelay(1000);
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
		pdi_mdelay(200);
		beep_off();
		led_off(LED_GREEN);
		led_off(LED_RED);
		pdi_mdelay(100);
	}
	for(int i = 0; i < 4; i++) {
		led_on(LED_GREEN);
		led_on(LED_RED);
		pdi_mdelay(200);
		led_off(LED_GREEN);
		led_off(LED_RED);
		pdi_mdelay(100);
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

static int pdi_GetDID(char did, char *data)
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

static int check_barcode(void)
{
	int try_times = 5;

	printf("##START##EC-checking barcode##END##\n");

	while (pdi_GetDID(0xB4, pdi_data_buf)) {
		try_times --;
		if (try_times < 0)
			return 1;
	}
	pdi_data_buf[16] = '\0';
	printf("##START##RB-");
	printf(pdi_data_buf);
	printf("##END##\n");
	if (memcmp(bcode_1 + 3, pdi_data_buf, 16))
		return 1;
	else
		return 0;
}

static int pdi_clear_dtc()
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

static int pdi_GetFault(char *data, int * pnum_fault)
{
	int i = 0, result = 0;
	if (pdi_GetDID(0x71, data))
		return 1;

	memset(data + 90, 0x00, 38);

	for (i = 0; i < 45; i += 3) {
		if (data[i] | data[i+1] | data [i + 2])
			result ++;
	}

	* pnum_fault = result;

	return 0;
}

static int pdi_wakeup()
{
	int i , rate;
	char temp[2];
	printf("##START##EC-Initialize LAN  communication##END##\n");
	pdi_batt_on();
//delay 400ms
	for(rate = 0; rate < 10; rate ++) {
		pdi_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}
	pdi_can_bus->send(&pdi_wakeup_msg);
//delay 100ms
	for(rate = 10; rate < 13; rate ++) {
		pdi_mdelay(30);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}
//try 2 times
	pdi_GetDID(0x22, pdi_data_buf);
	for(i = 0; i < 2; i++) {
		printf("%2x,", pdi_data_buf[i]&0xff);
	}
	printf("\n");
	pdi_GetDPID(0x05, pdi_data_buf);
	for(i = 0; i < 8; i++) {
		printf("%2x,", pdi_data_buf[i]&0xff);
	}
	printf("\n");
	pdi_mdelay(100);
	printf("##START##EC-Send High Voltage Wakeup##END##\n");
	pdi_can_bus->send(&pdi_wakeup_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_wakeup_msg);
	pdi_mdelay(20);
	printf("##START##EC-Transmitting Power Mode##END##\n");
	pdi_can_bus->send(&pdi_10_power_cfg_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_10_power_cfg_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_power_cfg_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_power_cfg_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_621_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_621_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_10_pwr_on_msg2);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_10_pwr_on_msg2);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_pwr_on_msg2);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_pwr_on_msg2);
	pdi_mdelay(20);
	printf("##START##STATUS-15##END##\n");
	pdi_can_bus->send(&pdi_10_pwr_on_msg1);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_10_pwr_on_msg1);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_pwr_on_msg1);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_11_pwr_on_msg1);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_dtc_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_dtc_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_cpid_msg);
	pdi_mdelay(20);
	pdi_can_bus->send(&pdi_cpid_msg);
	pdi_mdelay(20);
	printf("##START##STATUS-18##END##\n");
	pdi_mdelay(40);
	printf("##START##EC-ECU will be ready##END##\n");

	pdi_can_bus->send(&pdi_present_msg);
	pdi_can_bus->send(&pdi_present_msg);

	for(rate = 19; rate < 57; rate ++) {
		pdi_mdelay(80);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}
	pdi_can_bus->send(&pdi_present_msg);
	pdi_can_bus->send(&pdi_present_msg);

	for(rate = 57; rate < 95; rate ++) {
		pdi_mdelay(80);
		printf("##START##STATUS-");
		sprintf(temp,"%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}
	pdi_can_bus->send(&pdi_present_msg);
	pdi_can_bus->send(&pdi_present_msg);

	//pdi_mdelay(3000);感觉这里有错，应该再次发一下pdi_present_msg，不然后面会死掉
	//try DPID $12 again

	pdi_GetDID(0x71, pdi_data_buf);
	for(i = 0; i < 18; i ++)
		printf("%2x,", pdi_data_buf[i]&0xff);

	printf("\n");
	//pdi_sleep();

	return 0;
}

static int pdi_check_init(const struct pdi_cfg_s *sr)
{
	char *o=(char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	return 0;
}

static int pdi_check(const struct pdi_cfg_s *sr)
{
	int i, try_times = 5;
	int num_fault;
	const struct pdi_rule_s* pdi_cfg_rule;

	if(pdi_wakeup())
		return 1;

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
			while (pdi_GetDID(pdi_cfg_rule->para, pdi_data_buf)) {
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
			while (pdi_GetDPID(pdi_cfg_rule->para, pdi_data_buf)) {
				try_times --;
				if (try_times <= 0) {
					printf("##START##EC-read DPID:");
					printf("%X", (char *)pdi_cfg_rule->para);
					printf(" error ##END##\n");
					return 1;
				}
			}
			break;
		case PDI_RULE_UNDEF:
			return 1;
		}

		if(pdi_verify(pdi_cfg_rule, pdi_data_buf) == 0) {
			if(i == 12)
				pdi_can_bus->send(&pdi_present_msg);
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
	return 0;
}

static int pdi_sleep()
{
	char temp[3];
	int rate;
	printf("##START##EC---------------------------------------##END##\n");
	pdi_mdelay(20);
	printf("##START##EC-Transmitting Power Mode Off##END##\n");
	pdi_can_bus->send(&pdi_10_pwr_off_msg1);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_10_pwr_off_msg1);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_10_pwr_off_msg2);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_10_pwr_off_msg2);
	pdi_mdelay(10);
	printf("##START##STATUS-93##END##\n");
	pdi_can_bus->send(&pdi_11_pwr_off_msg1);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_11_pwr_off_msg1);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_11_pwr_off_msg2);
	pdi_mdelay(10);
	pdi_can_bus->send(&pdi_11_pwr_off_msg2);
	printf("##START##STATUS-94##END##\n");
	for(rate = 95; rate <= 100; rate ++) {
		pdi_mdelay(50);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}
	pdi_batt_off();
	return 0;
}

void pdi_init(void)
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
}

void pdi_process(void)
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

int main(void)
{
	ulp_init();
	pdi_init();
	init_OK();
	while(1) {
		pdi_process();
		ulp_update();
	}
}

static int cmd_sdm10_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"sdm10 , usage:\n"
		"sdm10 fault\n"
		"sdm10 clear\n"
		"sdm10 wakeup	it maybe result in some error code!\n"
		"sdm10 batt on/off\n"
		"sdm10 IGN on/off\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][0] == 'f') {
			pdi_wakeup();
			if (pdi_GetFault(pdi_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*3; i += 3)
					printf("0x%2x, 0x%2x, 0x%2x\n", pdi_fault_buf[i]&0xff, pdi_fault_buf[i+1]&0xff, pdi_fault_buf[i+2]&0xff);
			}
			pdi_sleep();
		}
		if(argv[1][0] == 'c') {
			pdi_wakeup();
			pdi_clear_dtc();
			pdi_sleep();
			printf("##OK##\n");
		}
		if(argv[1][0] == 'w') {
			pdi_wakeup();
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

const cmd_t cmd_sdm10 = {"sdm10", cmd_sdm10_func, "sdm10 cmd i/f"};
DECLARE_SHELL_CMD(cmd_sdm10)
