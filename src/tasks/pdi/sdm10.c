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
void pdi_process();

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
	//char temp[3];
	// for(int j = 90; j <= 100; j++) {
		// pdi_mdelay(50);
		// printf("##START##STATUS-");
		// sprintf(temp,"%d",j);
		// printf("%s",temp);
		// printf("##END##\n");
	// }
	pdi_mdelay(10);
	led_pass_on();
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
	for(int i = 0; i < 4; i++) {
		beep_on();
		led_pass_on();
		led_fail_on();
		pdi_mdelay(200);
		beep_off();
		led_pass_off();
		led_fail_off();
		pdi_mdelay(100);
	}
	for(int i = 0; i < 4; i++) {
		led_pass_on();
		led_fail_on();
		pdi_mdelay(200);
		led_pass_off();
		led_fail_off();
		pdi_mdelay(100);
	}
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
		if (msg_res.data[1] == 0x5a)
			memcpy(data, (msg_res.data + 3), msg_res.data[0] - 2);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 4), 4);
		printf("%d\n",data);
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

static int check_barcode()
{
	int n;
	char data[16];
	pdi_GetDID(0xB4,data);
	printf("##START##EC-checking barcode##END##\n");
	for(n = 3; n < 20; n++)
		if(bcode_1[n] == data[n-3])
			n++;
	if(n != 21) {
		return 0;
	}
	return 1;
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

static int pdi_wakeup()
{
	int i;
	char temp[2];
	printf("##START##EC-Initialize LAN  communication##END##\n");
	pdi_batt_on();
	for(int a = 0; a < 10; a++) {
		pdi_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp,"%d",a);
		printf("%s",temp);
		printf("##END##\n");
	}
	pdi_can_bus->send(&pdi_wakeup_msg);
	for(int b = 10; b <= 12; b++) {
		pdi_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp,"%d",b);
		printf("%s",temp);
		printf("##END##\n");
	}
	//pdi_mdelay(100);
	pdi_GetDID(0x22,pdi_data_buf);
	for(i = 0; i < 2; i++) {
		printf("%2x, ",pdi_data_buf[i]&0xff);
	}
	printf("\n");
	pdi_GetDPID(0x05,pdi_data_buf);
	for(i = 0; i < 8; i++) {
		printf("%2x, ",pdi_data_buf[i]&0xff);
	}
	printf("\n");
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
	pdi_can_bus->send(&pdi_present_msg);
	pdi_mdelay(25);
	printf("##START##EC-ECU will be ready##END##\n");
	for(int c = 19; c < 57; c++) {
		pdi_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp,"%d",c);
		printf("%s",temp);
		printf("##END##\n");
	}
	pdi_can_bus->send(&pdi_present_msg);
	for(int d = 57; d < 93; d++) {
		pdi_mdelay(40);
		printf("##START##STATUS-");
		sprintf(temp,"%d",d);
		printf("%s",temp);
		printf("##END##\n");
	}
	// pdi_can_bus->send(&pdi_present_msg);
	// pdi_mdelay(3000);
	pdi_can_bus->send(&pdi_present_msg);

	pdi_GetDPID(0x12,pdi_data_buf);
	for(i = 0; i < 8; i++) {
		printf("%2x, ",pdi_data_buf[i]&0xff);
	}
	printf("\n");
	//pdi_sleep();

	return 0;
}

static int pdi_check(const struct pdi_cfg_s *sr)
{
	int i, try_times;
	//char data[16];
	const struct pdi_rule_s* pdi_cfg_rule;
	char *o=(char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
		// for(int k = 0; k < 90; k++) {
		// pdi_mdelay(70);
		// printf("##START##STATUS-");
		// sprintf(temp,"%d",k);
		// printf("%s",temp);
		// printf("##END##\n");
	// }
	pdi_wakeup();
	if(check_barcode() == 0) {
		printf("##START##EC-barcode is wrong##END##\n");
		return 1;
	}
	for(i = 0; i < sr->nr_of_rules; i++) {
		try_times = 5;
		pdi_cfg_rule = pdi_rule_get(sr,i);
		if (&pdi_cfg_rule == NULL) {
			printf("##START##EC-no this rule##END##\n");
			return 1;
		}
		switch(pdi_cfg_rule->type) {
		case PDI_RULE_DID:
			printf("##START##EC-checking DID:");
			printf("%X",(char *)pdi_cfg_rule->para);
			printf("##END##\n");
			if(i == 0) {

			}
			while (pdi_GetDID(pdi_cfg_rule->para, pdi_data_buf)) {
				try_times --;
				if (try_times <= 0)
					return 1;
			}
			break;
		case PDI_RULE_DPID:
			printf("##START##EC-checking DPID:");
			printf("%X",(char *)pdi_cfg_rule->para);
			printf("##END##\n");
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
				printf("##START##RB-	");
				printf(pdi_data_buf);
				printf("##END##\n");
				// printf("##START##EC-checking barcode##END##\n");
				// for(i = 3; i < 20; i++)
					// if(bcode_1[i] == pdi_data_buf[i-3])
						// i++;
				// if(i != 21) {
					// printf("##START##EC-barcode is wrong##END##\n");
					// return 1;
				// }

			}
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
	for(int e = 95; e <= 100; e++) {
		pdi_mdelay(50);
		printf("##START##STATUS-");
		sprintf(temp,"%d",e);
		printf("%s",temp);
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

	pdi_drv_Init();
	mbi5025_Init(&pdi_mbi5025);
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);
	pdi_swcan_mode();
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
}

void pdi_process(void)
{
	int num_fault;
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[20];
	if(target_on() == 1)
		start_botton_on();
	else start_botton_off();
	if(ls1203_Read(&pdi_ls1203,bcode) == 0) {
		start_botton_off();
		led_flash(LED_GREEN);
		led_flash(LED_RED);
		bcode[19] = '\0';
		for(int i = 0; i<20 ;i++)
			bcode_1[i] = bcode[i];
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[9] = '\0';
		pdi_cfg_file = pdi_cfg_get(bcode);
		if(pdi_cfg_file == NULL) {
			pdi_sleep();
			led_off(LED_GREEN);
			led_off(LED_RED);
			pdi_fail_action();
			printf("##START##EC-no this config file##END##\n");
		} else {
			// led_flash(LED_GREEN);
			// led_flash(LED_RED);
			if(target_on() == 1) {
				if(pdi_check(pdi_cfg_file) == 0) {
					if(pdi_GetFault(pdi_fault_buf, &num_fault))
						printf("##START##EC-read DTC error##END##\n");
					else if(num_fault == 0) {
						pdi_sleep();
						led_off(LED_GREEN);
						led_off(LED_RED);
						//printf("##START##EC-no error##END##\n");
						pdi_pass_action();
						}
					else {
						pdi_clear_dtc();
						printf("##START##EC-");
						printf("num of fault is: %d*", num_fault);
						for (int i = 0; i < num_fault*3; i += 3)
							printf("0x%2x, 0x%2x, 0x%2x*", pdi_fault_buf[i]&0xff, pdi_fault_buf[i+1]&0xff, pdi_fault_buf[i+2]&0xff);
						printf("##END##\n");
						pdi_sleep();
						led_off(LED_GREEN);
						led_off(LED_RED);
						pdi_fail_action();
					}
				} else {
					pdi_sleep();
					led_off(LED_GREEN);
					led_off(LED_RED);
					pdi_fail_action();
					}
			} else {
				led_off(LED_GREEN);
				led_off(LED_RED);
				target_noton_action();
				printf("##START##EC-target is not on the right position##END##\n");
			}
			// led_off(LED_GREEN);
			// led_off(LED_RED);
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

static int cmd_pdi_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"pdi power control, usage:\n"
		"pdi on\n"
		"pdi off\n"
		"pdi fault\n"
		"pdi clear\n"
		"pdi wakeup\n"
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

const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi cmd i/f"};
DECLARE_SHELL_CMD(cmd_pdi)
