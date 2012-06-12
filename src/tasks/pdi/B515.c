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

#define PDI_DEBUG	0

//pdi_B515 can msg
static const can_msg_t b515_clrdtc_msg	=	{0x737, 8, {0x04, 0x14, 0xff, 0xff, 0xff, 0, 0, 0}, 0};
static const can_msg_t b515_errcode_msg	=	{0x737, 8, {0x03, 0x19, 0x02, 0x0a, 0, 0, 0, 0}, 0};
static const can_msg_t b515_faultcode_msg=	{0x737, 8, {0x03, 0x22, 0xfd, 0x39, 0, 0, 0, 0}, 0};
static const can_msg_t b515_getsn_msg	=	{0x737, 8, {0x03, 0x22, 0xfd, 0x3c, 0, 0, 0, 0}, 0};
static const can_msg_t b515_reqseed_msg	=	{0x737, 8, {0x02, 0x27, 0x61, 0, 0, 0, 0, 0}, 0};
static const can_msg_t b515_start_msg1	=	{0x737, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0};

static const mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};

static const ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 12,
		.dead_time = 20,
};

static const can_bus_t* pdi_can_bus = &can1;
static can_msg_t pdi_msg_buf[32];		//for multi frame buffer
static char b515_data_buf[256];			//data buffer
static char b515_fault_buf[64];			//fault buffer
static char bcode_1[12];

static int b515_check();
static int b515_init_OK();
static int b515_clear_dtc();
static int b515_mdelay(int );
static int b515_StartSession();
static int b515_check_barcode();
static int b515_GetCID(short cid, char *data);
static int b515_check_init(const struct pdi_cfg_s *);
static int b515_GetFault(char *data, int * pnum_fault);
static int pdi_pass_action();
static int pdi_fail_action();
static int pdi_led_start();
static int target_noton_action();
static int counter_pass_add();
static int counter_fail_add();
static void b515_process();
static void b515_CalculateKey(unsigned char , unsigned char parameter_1[8], unsigned char parameter_2[3]);

/**************************************************************************/
/************         Local funcitons                         *************/
/**************************************************************************/

static int b515_mdelay(int ms)
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

static void b515_CalculateKey(unsigned char accessLevel, unsigned char challenge[8], unsigned char secKeyOut[3])
{
	unsigned char idx, i;
	unsigned char mask = 0x01, x = 0;
	unsigned char kSecB2[2] = {0x00, 0x80};
	unsigned char kSecC2[2] = {0x00, 0x10};
	unsigned char kSecC1[2] = {0x00, 0x90};
	unsigned char kSecC0[2] = {0x00, 0x28};
	unsigned char secKey[3] = {0xA9, 0x41, 0xC5};

	// Select the five fixed bytes
	switch(accessLevel) {
		case 0x01:
			challenge[3] = 0x52;
			challenge[4] = 0x6F;
			challenge[5] = 0x77;
			challenge[6] = 0x61;
			challenge[7] = 0x6E;
			break;

		case 0x03:
			challenge[3] = 0x5A;
			challenge[4] = 0x89;
			challenge[5] = 0xE4;
			challenge[6] = 0x41;
			challenge[7] = 0x72;
			break;

		case 0x61:
			challenge[3] = 0x4D;
			challenge[4] = 0x5A;
			challenge[5] = 0x04;
			challenge[6] = 0x68;
			challenge[7] = 0x38;
			break;
	}

	// Key calculation
	for(i = 0; i < 64; i ++) {
		if((mask & challenge[x]) !=0)
			idx = 0x01;
		else idx = 0x00;
		idx ^= (secKey[0] & 0x01);
		if(mask == 0x80) {
			mask = 0x01;
			x ++;
		} else mask <<= 1;

		secKey[0] >>= 1;
		if((secKey[1] & 0x01) != 0)
			secKey[0] |= 0x80;

		secKey[1] >>= 1;
		if((secKey[2] & 0x01) != 0)
			secKey[1] |= 0x80;

		secKey[2] >>= 1;
		secKey[2] = (secKey[2] | kSecB2[idx]);
		secKey[0] = (secKey[0] ^ kSecC0[idx]);
		secKey[1] = (secKey[1] ^ kSecC1[idx]);
		secKey[2] = (secKey[2] ^ kSecC2[idx]);
	}

	secKeyOut[0] = (secKey[0] >> 4);
	secKeyOut[0] |= (secKey[1] << 4);

	secKeyOut[1] = (secKey[2] >> 4);
	secKeyOut[1] |= (secKey[1] & 0xF0);

	secKeyOut[2] = (secKey[2] & 0x0F);
	secKeyOut[2] |= (secKey[0] << 4);
}

//for start the session
static int b515_StartSession(void)
{
	int i, msg_len, num_fault;
	unsigned char seed[8], sendkey[3];
	can_msg_t msg;
	can_msg_t sendkey_msg = {
		0x737,
		8,
		{0x05, 0x27, 0x62, 0xff, 0xff, 0xff, 0, 0},
		0
	};
	//for start the required session
	if (usdt_GetDiagFirstFrame(&b515_start_msg1, 1, NULL, &msg, &msg_len))		//start session
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif
	//send seed require msg and seed saved in &msg
	if (usdt_GetDiagFirstFrame(&b515_reqseed_msg, 1, NULL, &msg, &msg_len))		//req seed
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

	//calculate the key from seed
	seed[0] = (unsigned char)msg.data[3];
	seed[1] = (unsigned char)msg.data[4];
	seed[2] = (unsigned char)msg.data[5];
	b515_CalculateKey(0x61, seed, sendkey);
	sendkey_msg.data[3] = sendkey[0];
	sendkey_msg.data[4] = sendkey[1];
	sendkey_msg.data[5] = sendkey[2];

	//send keys
	if (usdt_GetDiagFirstFrame(&sendkey_msg, 1, NULL, &msg, &msg_len))
		return 1;
	//judge the send key response
	if ((msg.data[1] != 0x67) || (msg.data[2] != 0x62))
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

#if PDI_DEBUG
	//get serial number
	printf("\nSN Code:\n");
	usdt_GetDiagFirstFrame(&b515_getsn_msg, 1, NULL, pdi_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(pdi_msg_buf + i, "\n");

	// get error code
	printf("\nError Code:\n");
	usdt_GetDiagFirstFrame(&b515_errcode_msg, 1, NULL, pdi_msg_buf, &msg_len);
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(pdi_msg_buf + i, "\n");

	if (b515_GetFault(b515_fault_buf, &num_fault))
		printf("##ERROR##\n");
	else {
		printf("##OK##\n");
		printf("num of fault is: %d\n", num_fault);
		for (i = 0; i < num_fault*4; i += 4)
			printf("0x%2x, 0x%2x, 0x%2x, 0x%2x\n",
			b515_fault_buf[i]&0xff, b515_fault_buf[i+1]&0xff,
			b515_fault_buf[i+2]&0xff, b515_fault_buf[i+3]&0xff);
	}

	//clear all include error code
	printf("\nClear all:\n");
	usdt_GetDiagFirstFrame(&b515_clrdtc_msg, 1, NULL, &msg, &msg_len);
	can_msg_print(&msg, "\n");

#endif

	return 0;
}

static int b515_GetCID(short cid, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x737, 8, {0x03, 0x22, 0, 0, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = (char)(cid >> 8);
	pdi_send_msg.data[3] = (char)(cid & 0x00ff);
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		pdi_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len))
			return 1;
	}

	//pick up datas
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x62)
			memcpy(data, (msg_res.data + 4), msg_res.data[0] - 3);
		else return 1;
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

static int b515_init_OK()
{
	led_on(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	b515_mdelay(200);
	led_off(LED_RED);
	led_off(LED_GREEN);
	beep_off();
	b515_mdelay(100);
	for(int i = 0; i < 5; i++) {
		led_on(LED_RED);
		led_on(LED_GREEN);
		b515_mdelay(200);
		led_off(LED_RED);
		led_off(LED_GREEN);
		b515_mdelay(100);
	}
	return 0;
}

static int counter_pass_add()
{
	counter_pass_rise();
	b515_mdelay(40);
	counter_pass_down();
	return 0;
}

static int counter_fail_add()
{
	counter_fail_rise();
	b515_mdelay(40);
	counter_fail_down();
	return 0;
}

static int pdi_fail_action()
{
	pdi_IGN_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_RED);
	counter_fail_add();
	beep_on();
	b515_mdelay(2000);
	beep_off();
	return 0;
}

static int pdi_pass_action()
{
	pdi_IGN_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	b515_mdelay(20);
	printf("##START##EC-Test Result : No Error ##END##\n");
	counter_pass_add();
	b515_mdelay(800);
	beep_off();
	b515_mdelay(150);
	beep_on();
	b515_mdelay(800);
	beep_off();
	return 0;
}

static int target_noton_action()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	for(int i = 0; i < 4; i ++) {
		beep_on();
		led_on(LED_GREEN);
		led_on(LED_RED);
		b515_mdelay(200);
		beep_off();
		led_off(LED_GREEN);
		led_off(LED_RED);
		b515_mdelay(100);
	}
	for(int i = 0; i < 4; i ++) {
		led_on(LED_GREEN);
		led_on(LED_RED);
		b515_mdelay(200);
		led_off(LED_GREEN);
		led_off(LED_RED);
		b515_mdelay(100);
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

static int b515_check_init(const struct pdi_cfg_s *sr)
{
	char *o = (char *)&(sr -> relay_ex);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	char *p = (char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(p+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+0));
	spi_cs_set(pdi_mbi5025.load_pin, 1);
	spi_cs_set(pdi_mbi5025.load_pin, 0);
	return 0;
}

static int b515_check_barcode()
{
	printf("##START##EC-Checking barcode...##END##\n");

	b515_GetCID(0xfd3c, b515_data_buf);

	b515_data_buf[12] = '\0';
	printf("##START##RB-");
	printf(b515_data_buf);
	printf("##END##\n");

	if (memcmp(b515_data_buf, bcode_1, 12))
		return 1;

	return 0;
}

static int b515_clear_dtc(void)
{
	int msg_len;
	can_msg_t msg;

	if (usdt_GetDiagFirstFrame(&b515_clrdtc_msg, 1, NULL, &msg, &msg_len))
		return 1;
	if (msg.data[1] != 0x54)	//positive response is 0x54
		return 1;

	return 0;
}

static int b515_GetFault(char *data, int * pnum_fault)
{
	int i,j, result = 0, msg_len;
	can_msg_t msg_res;

	memset(b515_fault_buf ,0x00 ,64);
	j = usdt_GetDiagFirstFrame(&b515_errcode_msg, 1, NULL, &msg_res, &msg_len);
	if(j) return 1;
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x59)
			memcpy(b515_fault_buf, (msg_res.data + 4), msg_res.data[0] - 3);
	}
	if (msg_len > 1) {
		pdi_msg_buf[0] = msg_res;
		usdt_GetDiagLeftFrame(pdi_msg_buf, msg_len);
		memcpy(b515_fault_buf, (msg_res.data + 5), 3);
		for (i = 1; i < msg_len; i++) {
			memcpy(&b515_fault_buf[3+(i-1)*7], (pdi_msg_buf + i)->data + 1, 7);
		}
	}
	memcpy(data, b515_fault_buf, 64);
	memset(data + 64, 0x00, 10);

	for (i = 0; i < 64; i += 4) {
		if (data[i] | data[i+1] | data[i+2] | data[i+3])
			result ++;
		else break;
	}

	* pnum_fault = result;

	return 0;
}

static int b515_check()
{
	int i, num_fault, try_times = 5, rate;
	char temp[2];

	pdi_IGN_on();
	//delay 1s
	for (rate = 10; rate <= 17; rate ++) {
		b515_mdelay(143);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	b515_StartSession();

	//check barcode
	if(b515_check_barcode()) {
		printf("##START##EC-Barcode Wrong##END##\n");
		return 1;
	}
	printf("##START##EC-      Checking barcode Done...##END##\n");

	printf("##START##EC-Waiting for ECU ready...##END##\n");
	//delay 9s
	for (rate = 17; rate <= 100; rate ++) {
		b515_mdelay(109);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	//b515_StartSession();

	//check error code
	while (b515_GetFault(b515_fault_buf, &num_fault)) {
		try_times --;
		if (try_times < 0) {
			printf("##START##EC-read DTC error##END##\n");
			return 1;
		}
	}

	if (num_fault) {
		//b515_clear_dtc();
		printf("##START##EC-");
		printf("num of fault is: %d\n", num_fault);
		for (i = 0; i < num_fault*4; i += 4)
			printf("0x%2x, 0x%2x, 0x%2x, 0x%2x\n",
			b515_fault_buf[i]&0xff, b515_fault_buf[i+1]&0xff,
			b515_fault_buf[i+2]&0xff, b515_fault_buf[i+3]&0xff);
		printf("##END##\n");
		return 1;
	}

	printf("##START##STATUS-100##END##\n");

	return 0;
}

void pdi_init(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 500000,
	};

	pdi_drv_Init();					//led,beep
	mbi5025_Init(&pdi_mbi5025);		//SPI总线	移位寄存器
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);		//scanner
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
}

static void b515_process(void)
{
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[20];
	if(target_on())
		start_botton_on();
	else start_botton_off();
	if(!ls1203_Read(&pdi_ls1203, bcode)) {

		start_botton_off();
		pdi_led_start();
		bcode[12] = '\0';

		memcpy(bcode_1, bcode, 12);
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[5] = '\0';

		pdi_cfg_file = pdi_cfg_get(bcode);

		if(target_on()) {
			if(pdi_cfg_file == NULL) {			//是否有此配置文件
				pdi_fail_action();
				printf("##START##EC-No This Config File##END##\n");
			} else {
				b515_check_init(pdi_cfg_file);		//relay config
				if(!b515_check()) pdi_pass_action();
				else pdi_fail_action();
				}
		} else {
			target_noton_action();
			printf("##START##EC-target is not on the right position or the pin protector is not placed##END##\n");
		}
	}
}

int main(void)
{
	ulp_init();
	pdi_init();
	b515_init_OK();
	while(1) {
		b515_process();
		ulp_update();
	}
}

static int cmd_b515_func(int argc, char *argv[])
{
	int num_fault, i;

	const char *usage = {
		"b515 , usage:\n"
		"b515 fault\n"
		"b515 clear\n"
		"b515 batt on/off\n"
		"b515 start, start the diagnostic seesion\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		if(argv[1][0] == 'f') {
			if (b515_GetFault(b515_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*4; i += 4)
					printf("0x%2x, 0x%2x, 0x%2x, 0x%2x\n",
					b515_fault_buf[i]&0xff, b515_fault_buf[i+1]&0xff,
					b515_fault_buf[i+2]&0xff, b515_fault_buf[i+3]&0xff);
			}
		}

		// start the diagnostic session
		if(argv[1][0] == 's') {
			b515_StartSession();
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

const cmd_t cmd_b515 = {"b515", cmd_b515_func, "b515 cmd i/f"};
DECLARE_SHELL_CMD(cmd_b515)

static int cmd_pdi_func(int argc, char *argv[])
{
	const char *usage = {
		"pdi , usage:\n"
		"pdi clear		clear the error code\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}
	//clear error code
	if(argc == 2) {
		if(argv[1][0] == 'c') {
			pdi_IGN_on();
			b515_mdelay(4000);
			b515_StartSession();
			if(b515_clear_dtc())
				printf("##ERROR##\n");
			else printf("##OK##\n");
			pdi_IGN_off();
		}
	}

	return 0;
}
const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi cmd i/f"};
DECLARE_SHELL_CMD(cmd_pdi)