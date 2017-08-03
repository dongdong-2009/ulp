/*
*
* miaofng@2017-07-16 routine for vwplc
* 1, vwplc system includes vwplc and vwhost
* 2, all cylinders are controled by vwhost, vwplc acts like a plc io extend module
* 3, handshake between test program and vwplc: rdy4tst/test_end/test_ng
* 4, vwplc can not request test unit ID
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "can.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "gpio.h"
#include <stdlib.h>

#define CAN_BAUD 500000
#define CAN_ID_HOST 0x100 //0x101..120 is test unit

enum {
	VWPLC_CMD_POLL, //host: cmd -> slave: response status
	VWPLC_CMD_SCAN, //host: cmd+rsel+csel
	VWPLC_CMD_MOVE, //host: cmd
	VWPLC_CMD_TEST, //host: cmd + mysql_record_id
	VWPLC_CMD_PASS, //host: cmd, identical with cmd "TEST PASS", only for debug purpose
	VWPLC_CMD_FAIL, //host: cmd, identical with cmd "TEST FAIL", only for debug purpose
	VWPLC_CMD_ENDC, //host: cmd, clear test_end request, indicates all cyl has been released
};

typedef struct {
	char target; //0: all slave
	char cmd; //assign, move, (get)status, ...
	char img; //used by move
	char msk; //used by move
	int sql_id; //used by test, test unit query the mysql of barcode through it
} rxdat_t;

typedef struct {
	char id; //test unit id, assign by host, range: 1-18
	char reserved1; //
	char reserved2;
	char reserved3;
	int sensors; //bit31: end, bit30: ng
} txdat_t;

static const can_bus_t *vwplc_can = &can1;
static can_msg_t vwplc_rxmsg;
static can_msg_t vwplc_txmsg;
static rxdat_t *vwplc_rxdat = (rxdat_t *) vwplc_rxmsg.data;
static txdat_t *vwplc_txdat = (txdat_t *) vwplc_txmsg.data;
static int vwplc_id;
static int vwplc_sql_id; //>= 0: rdy4tst
static int vwplc_test_end;
static int vwplc_test_ng;
static int vwplc_sensors;
static unsigned vwplc_guid;
static time_t vplc_mpc_on_timer;

static int vwplc_rimg(void)
{
	int img = gpio_rimg(0xffffffff);
	int msk = 0x0300ffff; //input
	img ^= msk;
	return img;
}

static void vwplc_wimg(int msk, int img)
{
	gpio_wimg(img, msk);
}

static void vwplc_can_handler(void)
{
	int rsel, csel, img, msk;
	if(vwplc_rxdat->target != 0) {
		if(vwplc_rxdat->target != vwplc_id) {
			if(vwplc_rxdat->cmd != VWPLC_CMD_SCAN)
				return;
		}
	}

	switch(vwplc_rxdat->cmd) {
	case VWPLC_CMD_SCAN:
		rsel = gpio_get("RSEL");
		csel = gpio_get("CSEL");
		if((rsel == 0) && (csel == 0)) {
			vwplc_id = vwplc_rxdat->target;
			vwplc_txmsg.id = CAN_ID_HOST + vwplc_id;
			vwplc_txmsg.dlc = 8;
		}
		break;

	case VWPLC_CMD_MOVE:
		img = vwplc_rxdat->img;
		msk = vwplc_rxdat->msk;
		vwplc_wimg(msk << 16, img << 16);
		break;

	case VWPLC_CMD_TEST:
		vwplc_sql_id = vwplc_rxdat->sql_id;
		break;

	case VWPLC_CMD_PASS:
		vwplc_test_ng = 0;
		vwplc_test_end = 1;
		break;

	case VWPLC_CMD_FAIL:
		vwplc_test_ng = 1;
		vwplc_test_end = 1;
		break;

	case VWPLC_CMD_ENDC:
		vwplc_test_end = 0;
		break;

	case VWPLC_CMD_POLL:
		if(vwplc_id) {
			int sensors = vwplc_sensors;
			sensors |= vwplc_test_end ? (1 << 31) : 0;
			sensors |= vwplc_test_ng ? (1 << 30) : 0;

			vwplc_txdat->id = vwplc_id;
			vwplc_txdat->sensors = sensors;
			vwplc_can->send(&vwplc_txmsg);
		}
		break;

	default:
		//only identified cmd to be responsed
		return;
	}
}

void vwplc_gpio_init(void)
{
	//sensors #00-07
	GPIO_BIND(GPIO_IPU, PC13, SC+) //GPI#00
	GPIO_BIND(GPIO_IPU, PC14, SC-) //GPI#01
	GPIO_BIND(GPIO_IPU, PC15, SR+) //GPI#02
	GPIO_BIND(GPIO_IPU, PA1, SR-) //GPI#03

	GPIO_BIND(GPIO_IPU, PA2, SF+) //GPI#04
	GPIO_BIND(GPIO_IPU, PA4, SF-) //GPI#05
	GPIO_BIND(GPIO_IPU, PA5, SB+) //GPI#06
	GPIO_BIND(GPIO_IPU, PA6, SB-) //GPI#07

	//sensors #10-17
	GPIO_BIND(GPIO_IPU, PA7, SP+) //GPI#10
	GPIO_BIND(GPIO_IPU, PC4, SP-) //GPI#11
	GPIO_BIND(GPIO_IPU, PC5, SM+) //GPI#12
	GPIO_BIND(GPIO_IPU, PB0, SM-) //GPI#13

	GPIO_BIND(GPIO_IPU, PB1, UE) //GPI#14
	GPIO_BIND(GPIO_IPU, PB2, GPI#15) //GPI#15
	GPIO_BIND(GPIO_IPU, PB10, GPI#16) //GPI#16
	GPIO_BIND(GPIO_IPU, PB11, GPI#17) //GPI#17

	//valve ctrl
	GPIO_BIND(GPIO_PP0, PB12, CC) //GPO#00
	GPIO_BIND(GPIO_PP0, PB13, CR) //GPO#01
	GPIO_BIND(GPIO_PP0, PB14, CF) //GPO#02
	GPIO_BIND(GPIO_PP0, PB15, CB) //GPO#03

	GPIO_BIND(GPIO_PP0, PC6, CP) //GPO#04
	GPIO_BIND(GPIO_PP0, PC7, CM) //GPO#05
	GPIO_BIND(GPIO_PP0, PC8, LR) //GPO#06
	GPIO_BIND(GPIO_PP0, PC9, LG) //GPO#07

	//misc
	GPIO_BIND(GPIO_IPU, PC12, RSEL)
	GPIO_BIND(GPIO_IPU, PC11, CSEL)

	GPIO_BIND(GPIO_PP0, PA8, MPC_ON)

	GPIO_BIND(GPIO_AIN, PC2, VMPC)
	GPIO_BIND(GPIO_AIN, PC3, VBST)
}

void vwplc_update(void)
{
	if (vplc_mpc_on_timer) {
		if(time_left(vplc_mpc_on_timer) < 0) {
			vplc_mpc_on_timer = 0;
			gpio_set("MPC_ON", 1);
		}
	}

	static time_t flash_timer = 0;

	//test end
	if(vwplc_test_end) {
		vwplc_sql_id = -1;
		gpio_set("LR", vwplc_test_ng ? 1 : 0);
		gpio_set("LG", vwplc_test_ng ? 0 : 1);
	}

	//test start
	if(vwplc_sql_id >= 0) {
		vwplc_test_end = 0;
		vwplc_test_ng = 0;

		//yellow flash
		if ((flash_timer == 0) || (time_left(flash_timer) < 0)) {
			flash_timer = time_get(1000);
			int lr = gpio_get("LR");
			gpio_set("LR", !lr);
			gpio_set("LG", !lr);
		}
	}

	vwplc_sensors = vwplc_rimg();
}

void vwplc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	const can_filter_t filter = {.id = CAN_ID_HOST, .mask = 0xffff, .flag = 0};
	memset(&vwplc_rxmsg, 0x00, sizeof(vwplc_rxmsg));
	memset(&vwplc_txmsg, 0x00, sizeof(vwplc_txmsg));
	vwplc_can->init(&cfg);
	vwplc_can->filt(&filter, 1);

	vwplc_gpio_init();

	//minpc power-on
	vwplc_guid = *(unsigned *)(0X1FFFF7E8);
	srand(vwplc_guid);
	int mdelay = rand() % 1000;
	printf("mpc_on delay = %03d mS\n", mdelay);
	vplc_mpc_on_timer = time_get(mdelay);

	/*!!! to usePreemptionPriority, group must be config first
	systick priority !must! use  NVIC_SetPriority() to set
	*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_SetPriority(SysTick_IRQn, 0);

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);

	//fixture init action
	vwplc_id = 0;
	vwplc_sql_id = -1;
	vwplc_test_ng = 0;
	vwplc_test_end = 1;
	vwplc_sensors = vwplc_rimg();

	gpio_set("CC", 0);
	gpio_set("CR", 0);
	gpio_set("CF", 0);
	gpio_set("CB", 0);
	gpio_set("CP", 0);
	gpio_set("CM", 0);
}

void main()
{
	sys_init();
	//shell_mute(NULL);
	vwplc_init();
	printf("vwplc v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		vwplc_update();
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while(!vwplc_can->recv(&vwplc_rxmsg)) {
		vwplc_can_handler();
	}
}

static int cmd_vwplc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"VWPLC ID?			return test unit id\n"
		"VWPLC GUID?		return test unit GUID\n"
		"VWPLC READY?		return -1 or mysql test record id\n"
		"VWPLC PASS\n"
		"VWPLC FAIL\n"
		"VWPLC RIMG?\n"
		"VWPLC WIMG MSK HEX\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(!strcmp(argv[1], "ID?")) {
		if(argc > 2) {
			sscanf(argv[2], "%d", &vwplc_id);
			vwplc_txmsg.id = CAN_ID_HOST + vwplc_id;
			vwplc_txmsg.dlc = 8;
		}
		printf("<%+d\n", vwplc_id);
	}
	else if(!strcmp(argv[1], "GUID?")) {
		printf("<%+d\n", vwplc_guid);
	}
	else if(!strcmp(argv[1], "READY?")) {
		printf("<%+d\n", vwplc_sql_id);
	}
	else if(!strcmp(argv[1], "PASS")) {
		vwplc_test_ng = 0;
		vwplc_test_end = 1;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "FAIL")) {
		vwplc_test_ng = 1;
		vwplc_test_end = 1;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "RIMG?")) {
		printf("<%+d, OK\n", vwplc_sensors);
	}
	else if(!strcmp(argv[1], "WIMG")) {
		int msk = 0;
		int img = 0;
		if(argc > 3) {
			int n = 0;
			if((argv[2][1] == 'x') || (argv[2][1] == 'X')) n += sscanf(argv[2], "%x", &msk);
			else n += sscanf(argv[2], "%d", &msk);
			if((argv[3][1] == 'x') || (argv[3][1] == 'X')) n += sscanf(argv[3], "%x", &img);
			else n += sscanf(argv[3], "%d", &img);
			if(n == 2) {
				vwplc_wimg(msk, img);
				printf("<%+d, OK\n", 0);
			}
		}
	}

	return 0;
}

cmd_t cmd_vwplc = {"VWPLC", cmd_vwplc_func, "vwplc i/f commands"};
DECLARE_SHELL_CMD(cmd_vwplc)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<0,Ulicar Technology,HUBPDI V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
