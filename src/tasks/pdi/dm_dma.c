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

#define PDI_DEBUG	0					//DEBUG MODEL

static const can_msg_t dm_clrdtc_msg =		{0x607, 8, {0x04, 0x2e, 0xfe, 0x90, 0xaa, 0, 0, 0}, 0};
#if PDI_DEBUG
static const can_msg_t dm_errcode_msg =		{0x607, 8, {0x03, 0x22, 0xfe, 0x80, 0, 0, 0, 0}, 0};
static const can_msg_t dm_getsn_msg =		{0x607, 8, {0x03, 0x22, 0xfe, 0x8d, 0, 0, 0, 0}, 0};
static const can_msg_t dm_part_msg =		{0x7D2, 8, {0x03, 0x22, 0xF1, 0x00, 0x55, 0x55, 0x55, 0x55}, 0};
#endif
static const can_msg_t dm_reqseed_msg =		{0x607, 8, {0x02, 0x27, 0x7d, 0, 0, 0, 0, 0}, 0};
static const can_msg_t dm_start_msg1 =		{0x607, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0};
static const can_msg_t dm_start_msg2 =		{0x7D2, 8, {0x02, 0x10, 0x03, 0, 0, 0, 0, 0}, 0};
static const can_msg_t dm_7D2_msg =			{0x7D2, 8, {0x30, 0, 0, 0, 0, 0, 0, 0},0};

static const mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};

static const ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 14,
		.dead_time = 20,
};

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

static struct can_queue_s dm_Vehicle_msg1 = {
	.ms = 1000,
	.msg = {0x5FA, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};
static struct can_queue_s dm_Vehicle_msg2 = {
	.ms = 1000,
	.msg = {0x5FC, 5, {0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};
static struct can_queue_s dm_ESC_msg = {
	.ms = 40,
	.msg = {0x2F0, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};
static struct can_queue_s dm_Testpresent_msg = {
	.ms = 500,
	.msg = {0x607, 8, {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static LIST_HEAD(can_queue);

static const can_bus_t* pdi_can_bus = &can1;

static can_msg_t dm_msg_buf[32];		//for multi frame buffer
static char dm_data_buf[256];			//data buffer
static char dm_fault_buf[64];			//fault buffer
static char bcode_1[14];
//static int esc_flag = 1;

static int dm_clear_dtc();
static int dm_mdelay(int );
//static int dm_esc_check();
static int dm_StartSession_1();
static int dm_StartSession_2();
static int dm_check_barcode();
static int dm_GetCID(short cid, char *data);
static int dm_check(const struct pdi_cfg_s *);
static int dm_GetFault(char *data, int * pnum_fault);
static int dm_check_init(const struct pdi_cfg_s *);
static int dm_part_check(const struct pdi_cfg_s *);
static int dm_error_check(char *, int *, const struct pdi_cfg_s *);
static int pdi_pass_action();
static int pdi_fail_action();
static int pdi_led_start();
static int pdi_init_OK();
static int target_noton_action();
static int counter_pass_add();
static int counter_fail_add();
static void dm_update();
static void dm_InitMsg();
static void dm_process();
static void dm_init();

/****************************************************************************/
/************				Local funcitons						*************/
/****************************************************************************/
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

static void dm_InitMsg(void)
{
	INIT_LIST_HEAD(&dm_Vehicle_msg1.list);
	list_add(&dm_Vehicle_msg1.list, &can_queue);

	INIT_LIST_HEAD(&dm_Vehicle_msg2.list);
	list_add(&dm_Vehicle_msg2.list, &can_queue);

	INIT_LIST_HEAD(&dm_ESC_msg.list);
	list_add(&dm_ESC_msg.list, &can_queue);

	INIT_LIST_HEAD(&dm_Testpresent_msg.list);
	list_add(&dm_Testpresent_msg.list, &can_queue);
}

static int dm_StartSession_1(void)
{
	int i, msg_len;
	can_msg_t msg;

	if(usdt_GetDiagFirstFrame(&dm_start_msg2, 1, NULL, &msg, &msg_len))
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
	printf("Part NO.:\n");
#endif
	return 0;
}

// for start the session
static int dm_StartSession_2(void)
{
	int i, msg_len, num_fault = 0;
	unsigned char seed[2], result;
	can_msg_t msg;
	can_msg_t sendkey_msg = {
		0x607,
		8,
		{0x04, 0x27, 0x7e, 0xff, 0xff, 0, 0, 0},
		0
	};

	if(usdt_GetDiagFirstFrame(&dm_start_msg1, 1, NULL, &msg, &msg_len))		//start session
		return 1;

#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

	if(usdt_GetDiagFirstFrame(&dm_reqseed_msg,1 , NULL, &msg, &msg_len))	//req seed
		return 1;

#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

	// calculate the key from seed
	seed[0] = (unsigned char)msg.data[3];
	seed[1] = (unsigned char)msg.data[4];
	result = seed[0] ^ seed[1];
	result ^= 0x23;
	result ^= 0x9a;
	sendkey_msg.data[3] = (char)((0x239a + result) >> 8);
	sendkey_msg.data[4] = (char)((0x239a + result) & 0x00ff);

	if (usdt_GetDiagFirstFrame(&sendkey_msg, 1, NULL, &msg, &msg_len))	//send key
		return 1;
	//judge the send key response
	if ((msg.data[1] != 0x67) || (msg.data[2] != 0x7e))
		return 1;
#if PDI_DEBUG
	can_msg_print(&msg, "\n");
#endif

#if PDI_DEBUG
	// get serial number
	printf("SN:\n");
	if(usdt_GetDiagFirstFrame(&dm_getsn_msg, 1, NULL, dm_msg_buf, &msg_len))
		return 1;
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(dm_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(dm_msg_buf + i, "\n");

	dm_GetFault(dm_fault_buf, &num_fault);
	printf("num of fault is: %d\n", num_fault);
	for (i = 0; i < num_fault*2; i += 2)
		printf("0x%2x, 0x%2x\n", dm_fault_buf[i]&0xff, dm_fault_buf[i+1]&0xff);
	memset(dm_fault_buf, 0, 52);

	// get error code
	printf("\nError Code:\n");
	if(usdt_GetDiagFirstFrame(&dm_errcode_msg, 1, NULL, dm_msg_buf, &msg_len))
		return 1;
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(dm_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(dm_msg_buf + i, "\n");

	// clear error code
	printf("\nClear Code:\n");
	if(usdt_GetDiagFirstFrame(&dm_clrdtc_msg, 1, NULL, dm_msg_buf, &msg_len))
		return 1;
	if (msg_len > 1)
		usdt_GetDiagLeftFrame(dm_msg_buf, msg_len);
	for (i = 0; i < msg_len; i++)
		can_msg_print(dm_msg_buf + i, "\n");
#endif

	return 0;
}

static int dm_GetCID(short cid, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x607, 8, {0x03, 0x22, 0xff, 0xff, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = (char)(cid >> 8);
	pdi_send_msg.data[3] = (char)(cid & 0x00ff);
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		dm_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(dm_msg_buf, msg_len))
			return 1;
	}

	// pick up the data
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x62)
			memcpy(data, (msg_res.data + 4), msg_res.data[0] - 3);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 5), 3);
		data += 3;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (dm_msg_buf + i)->data + 1, 7);
			data += 7;
		}
	}

	return 0;
}

static int dm_GetDID(short did, char *data)
{
	can_msg_t msg_res, pdi_send_msg = {0x7D2, 8, {0x03, 0x22, 0xff, 0xff, 0, 0, 0, 0}, 0};
	int i = 0, msg_len;

	pdi_send_msg.data[2] = (char)(did >> 8);
	pdi_send_msg.data[3] = (char)(did & 0x00ff);
	if (usdt_GetDiagFirstFrame(&pdi_send_msg, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		pdi_can_bus -> send(&dm_7D2_msg);
		dm_msg_buf[0] = msg_res;
		if(usdt_GetDiagLeftFrame(dm_msg_buf, msg_len))
			return 1;
	}

	// pick up the data
	if (msg_len == 1) {
		if (msg_res.data[1] == 0x62)
			memcpy(data, (msg_res.data + 4), msg_res.data[0] - 3);
		else
			return 1;
	} else if (msg_len > 1) {
		memcpy(data, (msg_res.data + 5), 3);
		data += 3;
		for (i = 1; i < msg_len; i++) {
			memcpy(data, (dm_msg_buf + i)->data + 1, 7);
			data += 7;
		}
	}

	return 0;
}

static int pdi_init_OK()
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
	pdi_IGN_off();
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
	pdi_IGN_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	dm_mdelay(20);
	printf("##START##EC-Test Result : No Error...##END##\n");
	counter_pass_add();
	dm_mdelay(650);
	beep_off();
	dm_mdelay(150);
	beep_on();
	dm_mdelay(650);
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

static int dm_check_init(const struct pdi_cfg_s *sr)
{
	char *o = (char *)&(sr -> relay_ex);
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

static int dm_error_check(char *fault, int *num, const struct pdi_cfg_s *sr)
{
	int i, j, flag;
	char dm_fault_temp[2];
	const struct pdi_rule_s* pdi_cfg_rule;
	for(i = 0; i < *num; i ++) {
		flag = 0;
		dm_fault_temp[0] = fault[i*2];
		dm_fault_temp[1] = fault[(i*2) + 1];
		for(j = 0; j < sr -> nr_of_rules; j ++) {
			pdi_cfg_rule = pdi_rule_get(sr, j);
			if(&pdi_cfg_rule == NULL) return 1;
			switch(pdi_cfg_rule -> type) {
				case PDI_RULE_ERROR:
					if(pdi_verify(pdi_cfg_rule, dm_fault_temp) == 0) {
						flag = 1;
						break;
					}
				case PDI_RULE_UNDEF:
					break;
			}
			if(flag == 1) break;
		}
		if(flag == 0) return 1;
	}
	return 0;
}

static int dm_part_check(const struct pdi_cfg_s *sr)
{
	int i;
	const struct pdi_rule_s* pdi_cfg_rule;
	dm_GetDID(0xf100, dm_data_buf);
	for(i = 0; i < sr -> nr_of_rules; i ++) {
		pdi_cfg_rule = pdi_rule_get(sr, i);
		if(&pdi_cfg_rule == NULL) return 1;
		switch(pdi_cfg_rule -> type) {
			case PDI_RULE_PART:
				if(pdi_verify(pdi_cfg_rule, dm_data_buf) == 0) return 0;
				else return 1;
			case PDI_RULE_UNDEF:
				break;
		}
	}
	printf("##START##EC-No Part NO. in Config Files...##END##\n");
	return 1;
}

// static int dm_esc_check()
// {
	// int a = 0, b = 0;
	// char temp;
	// can_msg_t esc_msg;
	// time_t deadtime = time_get(100);
	// printf("##START##EC-Checking ESC...##END##\n");
	// while(time_left(deadtime) > 0) {
		// pdi_can_bus -> recv(&esc_msg);
		// switch(esc_msg.id) {
		// case 0x188:
			// if(a == 1) break;
			// esc_msg.data[0] &= 0xF0;
			// if(esc_msg.data[0] != 0x00) return 1;
			// short v1 = esc_msg.data[1];
			// v1 <<= 8;
			// v1 |= (esc_msg.data[2] & 0xf0) ;
			// v1 >>= 4;
			// if(v1 < -20 || v1 > 20)
				// return 1;
			// short v2 = esc_msg.data[2] & 0x0F;
			// v2 <<= 8;
			// v2 |= esc_msg.data[3];
			// if(v2 < -63 || v2 > 63)
				// return 1;
			// if(esc_msg.data[5] != 0x00) return 1;
			// esc_msg.data[6] &= 0xF0;
			// if(esc_msg.data[6] != 0x00) return 1;
			// a = 1;
			// break;
		// case 0x189:
			// if(b == 0) break;
			// esc_msg.data[0] &= 0xF0;
			// if(esc_msg.data[0] != 0x00) return 1;
			// temp = (esc_msg.data[1] & 0xb0);
			// if(temp != 0x00) return 1;
			// short v3 = esc_msg.data[1] & 0x0F;
			// v3 <<= 8;
			// v3 |= esc_msg.data[3];
			// if(v3 < -63 || v3 > 63)
				// return 1;
			// b = 1;
			// break;
		// }
		// if((a == 1)&&(b == 1)) return 0;
		// else continue;
	// }
	// return 0;
// }

static int dm_check_barcode()
{
	int try_times = 5;
	printf("##START##EC-Checking barcode...##END##\n");

	while (dm_GetCID(0xfe8d, dm_data_buf)) {
		try_times --;
		if (try_times < 0)
			return 1;
	}

	dm_data_buf[14] = '\0';
	printf("##START##RB-");
	printf(dm_data_buf);
	printf("##END##\n");

	if(memcmp(dm_data_buf, bcode_1, 14))
		return 1;

	printf("##START##EC-      Checking barcode done...##END##\n");

	return 0;
}

static int dm_clear_dtc(void)
{
	int msg_len;
	can_msg_t msg;

	if (usdt_GetDiagFirstFrame(&dm_clrdtc_msg, 1, NULL, &msg, &msg_len))
		return 1;
	if (msg.data[1] != 0x6e)	//positive response is 0x6e
		return 1;
	return 0;
}

static int dm_GetFault(char *data, int * pnum_fault)
{
	int i, result = 0;

	if(dm_GetCID(0xfe80, data))
		return 1;

	for (i = 0; i < 52; i += 2) {
		if((data[i] | data[i+1]) == 0)
			break;
		if(data[i] == (char)(0xaa))
			break;
		result ++;
	}

	* pnum_fault = result;

	return 0;

}

static int dm_check(const struct pdi_cfg_s *sr)
{
	int i, num_fault = 0, try_times = 5, rate;
	char temp[2];

	pdi_IGN_on();

	printf("##START##EC-Waiting for checking part number...##END##\n");

	for (rate = 5; rate <= 25; rate ++) {
		dm_mdelay(180);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	dm_StartSession_1();

	if(dm_part_check(sr)) {
		dm_StartSession_1();
		if(dm_part_check(sr)) {
			dm_StartSession_1();
			if(dm_part_check(sr)) {
				printf("##START##EC-Part NO. error...##END##\n");
				return 1;
			}
		}
	}

	printf("##START##EC-      Checking part number done...##END##\n");

	printf("##START##EC-Waiting for checking others...##END##\n");

	//delay 6S
	for (rate = 25; rate <= 85; rate ++) {
		dm_mdelay(128);
		printf("##START##STATUS-");
		sprintf(temp, "%d", rate);
		printf("%s", temp);
		printf("##END##\n");
	}

	if(dm_StartSession_2()) {
		if(dm_StartSession_2()) {
			if(dm_StartSession_2()) {
				printf("##START##EC-Start error...##END##\n");
				return 1;
			}
		}
	}

	if(dm_check_barcode()) {
		printf("##START##EC-Barcode wrong...##END##\n");
		return 1;
	}

	// if(esc_flag) {
		// if(dm_esc_check()) {
			// printf("##START##EC-ESC error...##END##\n");
			// return 1;
		// }
	// }

	//printf("##START##EC-      Checking ESC done...##END##\n");
	//esc_flag = 1;

	if(dm_StartSession_2()) {
		if(dm_StartSession_2()) {
			if(dm_StartSession_2()) {
				printf("##START##EC-Start error...##END##\n");
				return 1;
			}
		}
	}

	while (dm_GetFault(dm_fault_buf, &num_fault)) {
		try_times --;
		if (try_times < 0) {
			printf("##START##EC-read DTC error...##END##\n");
			return 1;
		}
	}

	if (num_fault) {
		//dm_clear_dtc();
		dm_mdelay(20);
		printf("##START##EC-");
		printf("num of fault is: %d\n", num_fault);
		for (i = 0; i < num_fault*2; i += 2)
			printf("0x%2x, 0x%2x\n",
			dm_fault_buf[i]&0xff, dm_fault_buf[i+1]&0xff);
		printf("##END##\n");
		if(dm_error_check(dm_fault_buf, &num_fault, sr)) return 1;
		printf("##START##EC-These faults are acceptale...##END##\n");
	}

	printf("##START##STATUS-100##END##\n");

	return 0;
}

void dm_init(void)
{
	can_cfg_t cfg_pdi_can = {
		.baud = 500000,
	};

	pdi_drv_Init();						//led,beep
	mbi5025_Init(&pdi_mbi5025);			//SPI bus	shift register
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);			//scanner
	pdi_can_bus->init(&cfg_pdi_can);
	usdt_Init(pdi_can_bus);
	dm_InitMsg();
}

static void dm_process(void)
{
	const struct pdi_cfg_s* pdi_cfg_file;
	char bcode[15];
	if(target_on())
		start_botton_on();
	else start_botton_off();
	if(ls1203_Read(&pdi_ls1203, bcode) == 0) {

		start_botton_off();
		pdi_led_start();
		bcode[14] = '\0';

		memcpy(bcode_1, bcode, 14);
		printf("##START##SB-");
		printf(bcode,"\0");
		printf("##END##\n");
		bcode[5] = '\0';

		//some verites need esc check
		// if (memcmp(bcode + 3, "DL", 2) == 0)
			// esc_flag = 0;
		// if (memcmp(bcode + 3, "DK", 2) == 0)
			// esc_flag = 0;
		// if (memcmp(bcode + 3, "PK", 2) == 0)
			// esc_flag = 0;

		printf("##START##STATUS-5##END##\n");
		pdi_cfg_file = pdi_cfg_get(bcode);

		if(target_on()) {
			if(pdi_cfg_file == NULL) {			//whether or not the config file exist
				pdi_fail_action();
				printf("##START##EC-No this config file...##END##\n");
			} else {
				dm_check_init(pdi_cfg_file);	//relay config

				if(dm_check(pdi_cfg_file) == 0)
					pdi_pass_action();
				else
					pdi_fail_action();
			}
		} else {
			target_noton_action();
			printf("##START##EC-Target is not on the right position...##END##\n");
		}
	}
}

int main(void)
{
	ulp_init();
	dm_init();
	pdi_init_OK();
	while(1) {
		dm_process();
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
		"dm batt on/off\n"
		"dm start, start the diagnostic seesion\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argc == 2) {
		//read error code
		if(argv[1][0] == 'f') {
			dm_StartSession_2();
			if (dm_GetFault(dm_fault_buf, &num_fault))
				printf("##ERROR##\n");
			else {
				printf("##OK##\n");
				printf("num of fault is: %d\n", num_fault);
				for (i = 0; i < num_fault*2; i += 2)
					printf("0x%2x, 0x%2x\n",
					dm_fault_buf[i]&0xff, dm_fault_buf[i+1]&0xff);
			}
		}

		// start the diagnostic session
		if(argv[1][0] == 's') {
			dm_StartSession_2();
		}
	}

	//power on/off
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

static int cmd_pdi_func(int argc, char *argv[])
{
	const char *usage = {
		"pdi , usage:\n"
		"pdi clear\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}
	//clear error code
	if(argc == 2) {
		if(argv[1][0] == 'c') {
			pdi_IGN_on();
			dm_mdelay(2000);
			dm_StartSession_2();
			if (dm_clear_dtc())
				printf("##ERROR##\n");
			else
				printf("##OK##\n");
			pdi_IGN_off();
		}
	}

	return 0;
}
const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi cmd i/f"};
DECLARE_SHELL_CMD(cmd_pdi)
