/*
*
* miaofng@2017-07-16 routine for vwplc
* 1, vwplc system includes vwplc and vwhost
* 2, all cylinders are controled by vwhost, vwplc acts like a plc io extend module
* 3, handshake between test program and vwplc: rdy4tst/test_end/test_ng
* 4, vwplc can not request test unit ID
*
* miaofng@2017-10-01 arch change dueto poll mode too slow
* 1, cylinders are controlled by test unit, except pull/push by vwhost
* 2, vwplc send can msg to vwhost automatically in case of any sensor signal change
*
* miaofng@2018-02-20 port to fdplc/pwr system
* 1, add fdpwr board function into fdplc
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
#define CAN_ID_SCAN 0x200 //!low priority
#define CAN_ID_HOST 0x100 //0x101..120 is test unit
#define CAN_ID_UNIT(N) (CAN_ID_HOST + (N))
#define SENSOR_FLIT_MS 50

enum {
	FDPLC_CMD_POLL, //host: cmd -> slave: response status
	FDPLC_CMD_SCAN, //host: cmd+rsel+csel

	FDPLC_CMD_MOVE, //host: cmd
	FDPLC_CMD_TEST, //host: cmd + mysql_record_id

	//only for debug purpose, do not use it normal testing!!!!
	FDPLC_CMD_PASS, //host: cmd, identical with cmd "TEST PASS"
	FDPLC_CMD_FAIL, //host: cmd, identical with cmd "TEST FAIL"
	FDPLC_CMD_ENDC, //host: cmd, clear test_end request, indicates all cyl has been released
	FDPLC_CMD_WDTY, //host: cmd, enable test unit wdt
	FDPLC_CMD_WDTN, //host: cmd, disable test unit wdt
	FDPLC_CMD_UUID, //host: cmd, query fdplc board uuid
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
	char cmd;
	char reserved2;
	char reserved3;
	int sensors; //bit31: end, bit30: ng
} txdat_t;

static const can_bus_t *fdplc_can = &can1;
static can_msg_t fdplc_rxmsg;
static can_msg_t fdplc_txmsg;
static rxdat_t *fdplc_rxdat = (rxdat_t *) fdplc_rxmsg.data;
static txdat_t *fdplc_txdat = (txdat_t *) fdplc_txmsg.data;
static int fdplc_id;
static int fdplc_sql_id; //>= 0: rdy4tst
static int fdplc_test_end;
static int fdplc_test_ng;
static int fdplc_test_offline; /*vwhost will ignore offline mode test units*/
static int fdplc_test_died; /*only affect indicator*/
static time_t fdplc_test_wdt = 0;
#define FDPLC_TEST_WDT_MS 3000
static int fdplc_gpio_lr; //handle of gpio
static int fdplc_gpio_lg; //handle of gpio
static int fdplc_gpio_ly; //handle of gpio
static int fdplc_sensors;
static unsigned fdplc_guid;
static time_t vplc_mpc_on_timer;

static int fdplc_rimg(void)
{
	int img = gpio_rimg(0xffffffff);
	int msk = 0x0300ffff; //input
	img ^= msk;

	img &= 0x0FFFFFFF;
	img |= fdplc_test_end ? (1 << 31) : 0;
	img |= fdplc_test_ng ? (1 << 30) : 0;
	img |= fdplc_test_offline ? (1 << 29) : 0;
	img |= fdplc_test_died ? (1 << 28) : 0;
	return img;
}

static void fdplc_wimg(int msk, int img)
{
	gpio_wimg(img, msk);
}

static void fdplc_can_send(const can_msg_t *msg)
{
	led_hwSetStatus(LED_RED, LED_ON);
	while(fdplc_can->send(msg));
	led_hwSetStatus(LED_RED, LED_OFF);
}

static void fdplc_can_handler(void)
{
	int rsel, csel, img, msk;
	if(fdplc_rxdat->target != 0) {
		if(fdplc_rxdat->target != fdplc_id) {
			if(fdplc_rxdat->cmd != FDPLC_CMD_SCAN)
				return;
		}
	}

	switch(fdplc_rxdat->cmd) {
	case FDPLC_CMD_SCAN:
		rsel = gpio_get("RSEL");
		csel = gpio_get("CSEL");
		if((rsel == 0) && (csel == 0)) {
			fdplc_id = fdplc_rxdat->target;
			fdplc_txmsg.id = CAN_ID_UNIT(fdplc_id);
			fdplc_txmsg.dlc = 8;
		}

		//send the status to vwhost
		if(fdplc_id) {
			fdplc_txdat->id = fdplc_id;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->sensors = fdplc_sensors;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	case FDPLC_CMD_MOVE:
		img = fdplc_rxdat->img;
		msk = fdplc_rxdat->msk;
		fdplc_wimg(msk << 16, img << 16);
		break;

	case FDPLC_CMD_TEST:
		fdplc_sql_id = fdplc_rxdat->sql_id;
		break;

	case FDPLC_CMD_PASS:
		fdplc_test_ng = 0;
		fdplc_test_end = 1;
		break;

	case FDPLC_CMD_FAIL:
		fdplc_test_ng = 1;
		fdplc_test_end = 1;
		break;

	case FDPLC_CMD_ENDC:
		fdplc_test_end = 0;
		break;

	case FDPLC_CMD_WDTY:
		fdplc_test_died = 1;
		fdplc_test_wdt = 0;
		break;
	case FDPLC_CMD_WDTN:
		fdplc_test_died = 0;
		fdplc_test_wdt = 0;
		break;

	case FDPLC_CMD_UUID:
		if(fdplc_id) {
			unsigned uuid = *(unsigned *)(0X1FFFF7E8);
			fdplc_txdat->id = fdplc_id;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->sensors = uuid;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	case FDPLC_CMD_POLL:
		if(fdplc_id) {
			fdplc_txdat->id = fdplc_id;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->sensors = fdplc_sensors;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	default:
		//only identified cmd to be responsed
		return;
	}
}

void __sys_init(void)
{
	/*
	//sensors #00-07
	GPIO_BIND(GPIO_IPU, PC13, SC+) //0, GPI#00
	GPIO_BIND(GPIO_IPU, PC14, SC-) //1, GPI#01
	GPIO_BIND(GPIO_IPU, PC15, SR+) //2, GPI#02
	GPIO_BIND(GPIO_IPU, PA1, SR-) //3, GPI#03

	GPIO_FILT(SC+, SENSOR_FLIT_MS)
	GPIO_FILT(SC-, SENSOR_FLIT_MS)
	GPIO_FILT(SR+, SENSOR_FLIT_MS)
	GPIO_FILT(SR-, SENSOR_FLIT_MS)

	GPIO_BIND(GPIO_IPU, PA2, SF+) //4, GPI#04
	GPIO_BIND(GPIO_IPU, PA4, SF-) //5, GPI#05
	GPIO_BIND(GPIO_IPU, PA5, SB+) //6, GPI#06
	GPIO_BIND(GPIO_IPU, PA6, SB-) //7, GPI#07

	GPIO_FILT(SF+, SENSOR_FLIT_MS)
	GPIO_FILT(SF-, SENSOR_FLIT_MS)
	GPIO_FILT(SB+, SENSOR_FLIT_MS)
	GPIO_FILT(SB-, SENSOR_FLIT_MS)

	//sensors #10-17
	GPIO_BIND(GPIO_IPU, PA7, SP+) //8, GPI#10
	GPIO_BIND(GPIO_IPU, PC4, SP-) //9, GPI#11
	GPIO_BIND(GPIO_IPU, PC5, SM+) //10,GPI#12
	GPIO_BIND(GPIO_IPU, PB0, SM-) //11, GPI#13

	GPIO_FILT(SP+, SENSOR_FLIT_MS)
	GPIO_FILT(SP-, SENSOR_FLIT_MS)
	GPIO_FILT(SM+, SENSOR_FLIT_MS)
	GPIO_FILT(SM-, SENSOR_FLIT_MS)

	GPIO_BIND(GPIO_IPU, PB1, UE) //12, GPI#14
	GPIO_BIND(GPIO_IPU, PB2, GPI#15) //13, GPI#15
	GPIO_BIND(GPIO_IPU, PB10, GPI#16) //14, GPI#16
	GPIO_BIND(GPIO_IPU, PB11, GPI#17) //15, GPI#17

	GPIO_FILT(UE, SENSOR_FLIT_MS)

	//valve ctrl
	GPIO_BIND(GPIO_PP0, PB12, CC) //16, GPO#00
	GPIO_BIND(GPIO_PP0, PB13, CR) //17, GPO#01
	GPIO_BIND(GPIO_PP0, PB14, CF) //18, GPO#02
	GPIO_BIND(GPIO_PP0, PB15, CB) //19, GPO#03

	GPIO_BIND(GPIO_PP0, PC6, CP) //20, GPO#04
	GPIO_BIND(GPIO_PP0, PC7, CM) //21, GPO#05
	*/
	fdplc_gpio_lr = GPIO_BIND(GPIO_PP0, PA00, LR) //
	fdplc_gpio_lg = GPIO_BIND(GPIO_PP0, PA01, LG) //
	fdplc_gpio_ly = GPIO_BIND(GPIO_PP0, PA02, LY) //

	//misc
	//GPIO_BIND(GPIO_IPU, PC12, RSEL) //24,
	//GPIO_BIND(GPIO_IPU, PC11, CSEL) //25,
	//GPIO_FILT(RSEL, 10)
	//GPIO_FILT(CSEL, 10)

	GPIO_BIND(GPIO_PP0, PC00, LED_R)
	GPIO_BIND(GPIO_PP0, PC01, LED_G)
}

void fdplc_update(void)
{
	static time_t flash_timer = 0;
	int lr, lg, update = 0;

	//test end
	if(fdplc_test_end) {
		fdplc_sql_id = -1;
		lr = fdplc_test_ng ? 1 : 0;
		lg = fdplc_test_ng ? 0 : 1;
		update = 1;
	}

	//test start
	if(fdplc_sql_id >= 0) {
		fdplc_test_end = 0;
		fdplc_test_ng = 0;

		//yellow flash
		if ((flash_timer == 0) || (time_left(flash_timer) < 0)) {
			flash_timer = time_get(1000);
			lr = gpio_get_h(fdplc_gpio_lr);
			lr = !lr;
			lg = !lr;
			update = 1;
		}
	}

	//test unit died detection
	if(fdplc_test_wdt) {
		if(time_left(fdplc_test_wdt) < 0) {
			__disable_irq(); //to avoid confict with vwhost remote wdt disable
			if(fdplc_test_wdt) {
				fdplc_test_wdt = 0;
				fdplc_test_died = 1;
			}
			__enable_irq();
		}
	}

	//show constant yellow if test unit died
	if(fdplc_test_died) {
		lr = lg = 1;
		update = 1;
	}

	//update front panel indicator display
	if(update) {
		gpio_set_h(fdplc_gpio_lr, lr);
		gpio_set_h(fdplc_gpio_lg, lg);
	}

	int sensors = fdplc_rimg();
	int delta = sensors ^ fdplc_sensors;
	if(delta) {
		fdplc_sensors = sensors;

		#define MASK_EXCEPT_CESL_AND_RSEL (~0X03000000)
		if(delta & MASK_EXCEPT_CESL_AND_RSEL) {
			if(fdplc_id) {
				//report to vwhost
				__disable_irq();
				fdplc_txdat->id = fdplc_id;
				fdplc_txdat->cmd = FDPLC_CMD_POLL;
				fdplc_txdat->sensors = fdplc_sensors;
				fdplc_can_send(&fdplc_txmsg);
				__enable_irq();
			}
		}
	}
}

void fdplc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	const can_filter_t filter[] = {
		{.id = CAN_ID_HOST, .mask = 0xffff, .flag = 0},
		{.id = CAN_ID_SCAN, .mask = 0xffff, .flag = 0},
	};
	memset(&fdplc_rxmsg, 0x00, sizeof(fdplc_rxmsg));
	memset(&fdplc_txmsg, 0x00, sizeof(fdplc_txmsg));
	fdplc_can->init(&cfg);
	fdplc_can->filt(filter, 2);

	//minpc power-on
	fdplc_guid = *(unsigned *)(0X1FFFF7E8);
	srand(fdplc_guid);
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
	fdplc_id = 0;
	fdplc_sql_id = -1;
	fdplc_test_ng = 0;
	fdplc_test_end = 1;
	fdplc_sensors = fdplc_rimg();

	//global status
	fdplc_test_offline = 0;
	fdplc_test_died = 1;
	fdplc_test_wdt = 0;

	gpio_set("CC", 0);
	gpio_set("CR", 0);
	gpio_set("CF", 0);
	gpio_set("CB", 0);
	gpio_set("CP", 0);
	gpio_set("CM", 0);
}

//implemented by fdpwr.c
extern void fdpwr_init(void);
extern void fdpwr_update(void);

void main()
{
	sys_init();
	shell_mute(NULL);
	fdplc_init();
	fdpwr_init();
	printf("fdplc v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		fdplc_update();
		fdpwr_update();
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while(!fdplc_can->recv(&fdplc_rxmsg)) {
		fdplc_can_handler();
	}
}

static int cmd_fdplc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"FDPLC ID?			return test unit id\n"
		"FDPLC READY?		return -1 or mysql test record id\n"
		"FDPLC OFFLINE		change fdplc to offline mode\n"
		"FDPLC ONLINE		change fdplc to online mode\n"
		"FDPLC PASS\n"
		"FDPLC FAIL\n"
		"FDPLC RIMG?\n"
		"FDPLC WIMG MSK HEX\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(!strcmp(argv[1], "ID?")) {
		if(argc > 2) {
			sscanf(argv[2], "%d", &fdplc_id);
			fdplc_txmsg.id = CAN_ID_UNIT(fdplc_id);
			fdplc_txmsg.dlc = 8;
		}
		printf("<%+d\n", fdplc_id);
	}
	else if(!strcmp(argv[1], "READY?")) {
		printf("<%+d\n", fdplc_sql_id);
	}
	else if(!strcmp(argv[1], "OFFLINE")) {
		fdplc_test_offline = 1;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "ONLINE")) {
		fdplc_test_offline = 0;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "PASS")) {
		fdplc_test_ng = 0;
		fdplc_test_end = 1;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "FAIL")) {
		fdplc_test_ng = 1;
		fdplc_test_end = 1;
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "RIMG?")) {
		printf("<%+d, OK\n", fdplc_sensors);
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
				fdplc_wimg(msk, img);
				printf("<%+d, OK\n", 0);
			}
		}
	}

	return 0;
}

cmd_t cmd_fdplc = {"FDPLC", cmd_fdplc_func, "fdplc i/f commands"};
DECLARE_SHELL_CMD(cmd_fdplc)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"*UUID?		query the stm32 uuid\n"
		"*WDT?		update wdt and return ms left\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<0,Ulicar Technology, FDPLC V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*UUID?")) {
		unsigned uuid = *(unsigned *)(0X1FFFF7E8);
		printf("<%+d\n", uuid);
		return 0;
	}
	else if(!strcmp(argv[0], "*WDT?")) {
		int ms = (fdplc_test_wdt == 0) ? 0 : time_left(fdplc_test_wdt);
		if((fdplc_test_wdt == 0) && (fdplc_test_died == 0)) {
			//wdt been disabled by vwhost, ignore update here
		}
		else {
			fdplc_test_wdt = time_get(FDPLC_TEST_WDT_MS);
			fdplc_test_died = 0;
		}
		printf("<%+d\n", ms);
		return 0;
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
