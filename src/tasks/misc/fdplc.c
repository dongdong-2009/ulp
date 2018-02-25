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
static int fdiox_gpio_lx; //iox rj45 indicator
static time_t fdiox_lx_timer = 0;

static int fdplc_rimg(void)
{
	if((fdiox_lx_timer == 0) || (time_left(fdiox_lx_timer) < 0)){
		fdiox_lx_timer = time_get(500);
		gpio_inv_h(fdiox_gpio_lx);
	}

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
	const mcp23s17_t mcp_bus = {.bus = &spi1, .idx = SPI_1_NSS };
	gpio_mcp_init(&mcp_bus);

	//index 00-07, iox sensors
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA00, UE ) //IOX-IN01
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA01, S02) //IOX-IN02
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA02, SC+) //IOX-IN03
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA03, SC-) //IOX-IN04
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA04, SR+) //IOX-IN05
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA05, SR-) //IOX-IN06
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA06, SF+) //IOX-IN07
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA07, SF-) //IOX-IN08

	GPIO_FILT(UE , SENSOR_FLIT_MS)
	GPIO_FILT(S02, SENSOR_FLIT_MS)
	GPIO_FILT(SC+, SENSOR_FLIT_MS)
	GPIO_FILT(SC-, SENSOR_FLIT_MS)
	GPIO_FILT(SR+, SENSOR_FLIT_MS)
	GPIO_FILT(SR-, SENSOR_FLIT_MS)
	GPIO_FILT(SF+, SENSOR_FLIT_MS)
	GPIO_FILT(SF-, SENSOR_FLIT_MS)

	//index 08-15, iox sensors
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB00, SB+) //IOX-IN09
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB01, SB-) //IOX-IN10
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB02, S11) //IOX-IN11
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB03, S12) //IOX-IN12
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB04, S13) //IOX-IN13
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB05, S14) //IOX-IN14
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB06, S15) //IOX-IN15
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB07, S16) //IOX-IN16

	GPIO_FILT(SB+, SENSOR_FLIT_MS)
	GPIO_FILT(SB-, SENSOR_FLIT_MS)
	GPIO_FILT(S11, SENSOR_FLIT_MS)
	GPIO_FILT(S12, SENSOR_FLIT_MS)
	GPIO_FILT(S13, SENSOR_FLIT_MS)
	GPIO_FILT(S14, SENSOR_FLIT_MS)
	GPIO_FILT(S15, SENSOR_FLIT_MS)
	GPIO_FILT(S16, SENSOR_FLIT_MS)

	//index 16-19, plc sensors
	GPIO_BIND_INV(GPIO_IPU, PC14, SM+)
	GPIO_BIND_INV(GPIO_IPU, PC15, SM-)
	GPIO_BIND(GPIO_PP0, PC02, CM+)
	GPIO_BIND(GPIO_PP0, PC03, CM-)

	GPIO_FILT(SM+, SENSOR_FLIT_MS)
	GPIO_FILT(SM-, SENSOR_FLIT_MS)

	//index 20-27, iox valve ctrl
	GPIO_BIND(GPIO_PP0, mcp1:PA00, CC) //IOX-OUT01
	GPIO_BIND(GPIO_PP0, mcp1:PA01, CR) //IOX-OUT02
	GPIO_BIND(GPIO_PP0, mcp1:PA02, CF) //IOX-OUT03
	GPIO_BIND(GPIO_PP0, mcp1:PA03, CB) //IOX-OUT04
	GPIO_BIND(GPIO_PP0, mcp1:PA04, C5) //IOX-OUT05
	GPIO_BIND(GPIO_PP0, mcp1:PA05, C6) //IOX-OUT06
	GPIO_BIND(GPIO_PP0, mcp1:PA06, C7) //IOX-OUT07
	GPIO_BIND(GPIO_PP0, mcp1:PA07, C8) //IOX-OUT08

	//index 28-30, plc valve
	GPIO_BIND(GPIO_PP0, PA00, LR)
	GPIO_BIND(GPIO_PP0, PA01, LG)
	GPIO_BIND(GPIO_PP0, PA02, LY)

	//misc
	GPIO_BIND_INV(GPIO_IPU, PC10, RSEL)
	GPIO_BIND_INV(GPIO_IPU, PC11, CSEL)
	GPIO_FILT(RSEL, 10)
	GPIO_FILT(CSEL, 10)

	GPIO_BIND(GPIO_PP0, PC00, LED_R)
	GPIO_BIND(GPIO_PP0, PC01, LED_G)
	GPIO_BIND(GPIO_PP0, mcp1:PB00, LX) //iox rj45 indicator

	fdplc_gpio_lr = gpio_handle("LR"); //front face indicator
	fdplc_gpio_lg = gpio_handle("LG"); //front face indicator
	fdplc_gpio_ly = gpio_handle("LY"); //front face indicator
	fdiox_gpio_lx = gpio_handle("LX"); //iox rj45 indicator
}

void fdplc_update(void)
{
	static time_t flash_timer = 0;
	int lr, lg, ly, update = 0;

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
		if(1) {
			//3-color led process
			ly = 0;
			if((lr == 1) && (lg == 1)) {
				lr = lg = 0;
				ly = 1;
			}
		}
		gpio_set_h(fdplc_gpio_lr, lr);
		gpio_set_h(fdplc_gpio_lg, lg);
		gpio_set_h(fdplc_gpio_ly, ly);
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
