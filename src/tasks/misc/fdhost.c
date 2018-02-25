/*
*
* miaofng@2018-02-25 routine for ford
* 1, port from vwhost
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

#define CAN_BAUD 500000
#define CAN_ID_SCAN 0x200 //!low priority
#define CAN_ID_HOST 0x100 //0x101..120 is test unit
#define CAN_ID_UNIT(N) (CAN_ID_HOST + (N))

#define FD_SCAN_MS 500
#define FD_ALIVE_MS (FD_SCAN_MS << 2)

#define FD_SAFE_MASK (1 << 11) //!!! 'SM-'
static int fd_hgpio_safe = -1;
static int fd_flag_safe = -1;

#define FD_UUTE_MASK (1 << 12) //!!! 'UE'
static int fd_flag_uute = -1;

enum {
	FDPLC_CMD_POLL, //host: cmd -> slave: response status
	FDPLC_CMD_SCAN, //host: cmd+rsel+csel
	FDPLC_CMD_MOVE, //host: cmd
	FDPLC_CMD_TEST, //host: cmd + mysql_record_id
	FDPLC_CMD_PASS, //host: cmd, identical with cmd "TEST PASS", only for debug purpose
	FDPLC_CMD_FAIL, //host: cmd, identical with cmd "TEST FAIL", only for debug purpose
	FDPLC_CMD_ENDC, //host: cmd, clear test_end request, indicates all cyl has been released
	FDPLC_CMD_WDTY, //host: cmd, enable test unit wdt
	FDPLC_CMD_WDTN, //host: cmd, disable test unit wdt
	FDPLC_CMD_UUID, //host: cmd, query fdplc board uuid
};

typedef struct {
	unsigned target : 4;
	unsigned cmd : 4;
	union {
		struct {
			unsigned img : 16;
			unsigned msk : 16;
		} move;

		struct {
			int sql_id;
		} test;
	};
} txdat_t;

typedef struct {
	unsigned id : 4; //test unit id, assign by host
	unsigned cmd : 4;
	unsigned model: 8; //fixture model type
	unsigned counter : 16; //probe counter, low 16bit
	unsigned sensors; //bit31: end, bit30: ng
} rxdat_t;

#define NPLC 12
static const can_bus_t *fd_can = &can1;
static can_msg_t fd_rxmsg[NPLC + 1];
static can_msg_t fd_txmsg;
static rxdat_t *fd_rxdat = (rxdat_t *) fd_rxmsg[0].data;
static txdat_t *fd_txdat = (txdat_t *) fd_txmsg.data;
static int fdplc_uuid[NPLC + 1];

static int fdhost_can_send(can_msg_t *msg)
{
	const txdat_t *txdat = (const txdat_t *) msg->data;
	switch(txdat->cmd) {
	case FDPLC_CMD_SCAN:
	case FDPLC_CMD_POLL:
		msg->id = CAN_ID_SCAN;
		break;
	default:
		msg->id = CAN_ID_HOST;
		break;
	}

	led_hwSetStatus(LED_RED, 1);
	while(fd_can->send(msg));
	led_hwSetStatus(LED_RED, 0);
	return 0;
}

void __sys_tick(void)
{
	//can msg id is used as alive detection counter
	//set dlc=0 indicates the fixture is lost
	int uute = 0;
	for(int i = 1; i <= NPLC; i ++) {
		if(fd_rxmsg[i].id <= 0) fd_rxmsg[i].dlc = 0;
		else {
			fd_rxmsg[i].id --;
			rxdat_t *rxdat = (rxdat_t *) fd_rxmsg[i].data;
			if(rxdat->sensors & FD_UUTE_MASK) {
				uute |= 1 << i;
			}
		}
	}
	fd_flag_uute = uute;
}

static void fdhost_can_handler(void)
{
	int id = fd_rxdat->id;
	if(fd_rxdat->cmd == FDPLC_CMD_UUID) {
		fdplc_uuid[id] = fd_rxdat->sensors;
		return;
	}

	memcpy(&fd_rxmsg[id], &fd_rxmsg[0], sizeof(can_msg_t));

	//can msg id is used as alive detection counter
	fd_rxmsg[id].id = FD_ALIVE_MS;

	//danger process
	int safe = 1;
	for(int i = 1; i <= NPLC; i ++) {
		if(fd_rxmsg[i].dlc > 0) {
			rxdat_t *rxdat = (rxdat_t *) fd_rxmsg[i].data;
			if((rxdat->sensors & FD_SAFE_MASK) == 0) {
				safe = 0;
				break;
			}
		}
	}

	fd_flag_safe = safe;
	gpio_set_h(fd_hgpio_safe, fd_flag_safe);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while(!fd_can->recv(&fd_rxmsg[0])) {
		fdhost_can_handler();
	}
}

void fd_gpio_init(void)
{
	//sensors #00-07
	GPIO_BIND(GPIO_IPU, PC13, GPI#00) //GPI#00
	GPIO_BIND(GPIO_IPU, PC14, GPI#01) //GPI#01
	GPIO_BIND(GPIO_IPU, PC15, GPI#02) //GPI#02
	GPIO_BIND(GPIO_IPU, PA01, GPI#03) //GPI#03

	GPIO_BIND(GPIO_IPU, PA02, GPI#04) //GPI#04
	GPIO_BIND(GPIO_IPU, PA04, GPI#05) //GPI#05
	GPIO_BIND(GPIO_IPU, PA05, GPI#06) //GPI#06
	GPIO_BIND(GPIO_IPU, PA06, GPI#07) //GPI#07

	//sensors #10-17
	GPIO_BIND(GPIO_IPU, PA07, GPI#10) //GPI#10
	GPIO_BIND(GPIO_IPU, PC04, GPI#11) //GPI#11
	GPIO_BIND(GPIO_IPU, PC05, GPI#12) //GPI#12
	GPIO_BIND(GPIO_IPU, PB00, GPI#13) //GPI#13

	GPIO_BIND(GPIO_IPU, PB01, GPI#14) //GPI#14
	GPIO_BIND(GPIO_IPU, PB02, GPI#15) //GPI#15
	GPIO_BIND(GPIO_IPU, PB10, GPI#16) //GPI#16
	GPIO_BIND(GPIO_IPU, PB11, GPI#17) //GPI#17

	//valve ctrl
	GPIO_BIND(GPIO_PP0, PB12, RSEL0) //GPO#00
	GPIO_BIND(GPIO_PP0, PB13, RSEL1) //GPO#01
	GPIO_BIND(GPIO_PP0, PB14, RSEL2) //GPO#02
	GPIO_BIND(GPIO_PP0, PB15, RSEL3) //GPO#03

	GPIO_BIND(GPIO_PP0, PC06, CSEL0) //GPO#04
	GPIO_BIND(GPIO_PP0, PC07, CSEL1) //GPO#05
	GPIO_BIND(GPIO_PP0, PC08, CSEL2) //GPO#06
	fd_hgpio_safe = GPIO_BIND(GPIO_PP0, PC09, "SAFE") //GPO#07

	//misc
	GPIO_BIND(GPIO_IPU, PC12, ESTOP#) //RSEL
	GPIO_BIND(GPIO_IPU, PC11, RST#) //CSEL

	//GPIO_BIND(GPIO_PP0, PA08, MPC_ON)

	GPIO_BIND(GPIO_AIN, PC02, VMPC)
	GPIO_BIND(GPIO_AIN, PC03, VBST)
}

/*id range: [1, 12]*/
int fd_assign(int idx)
{
	sys_assert((idx >= 0) && (idx < NPLC));
	const int slaves[] = { //operator view
		0x1011, 0x2012, 0x3014,
		0x4021, 0x5022, 0x6024,
		0x7041, 0x8042, 0x9044,
		0xa081, 0xb082, 0xc084,
	};

	//convert idx to id
	int id = slaves[idx] >> 12;
	id &= 0x0f;

	static int last_idx = -1;
	if(last_idx >= 0) {
		//csel/rsel signals need stable time
		fd_txdat->target = id;
		fd_txdat->cmd = FDPLC_CMD_SCAN;
		fd_txmsg.dlc = 2;
		fdhost_can_send(&fd_txmsg);

		last_idx = -1;
		return 0; //id been assigned
	}

	char rsel = (slaves[idx] >> 4) & 0x0f;
	char csel = (slaves[idx] >> 0) & 0x0f;
	gpio_set("RSEL0", rsel & 0x01);
	gpio_set("RSEL1", rsel & 0x02);
	gpio_set("RSEL2", rsel & 0x04);
	gpio_set("RSEL3", rsel & 0x08);
	gpio_set("CSEL0", csel & 0x01);
	gpio_set("CSEL1", csel & 0x02);
	gpio_set("CSEL2", csel & 0x04);
	last_idx = idx;
	return id;
}

void fd_update(void)
{
	//so boring .. assign the fdplc id :(
	static int idx_scan = 0;
	static int scan_timer = 0;

	if((scan_timer == 0) || (time_left(scan_timer) < 0)) {
		scan_timer = time_get(FD_SCAN_MS >> 1);
		int idx = fd_assign(idx_scan);
		idx_scan += (idx == 0) ? 1 : 0;
		idx_scan = (idx_scan > 8) ? 0 : idx_scan;
	}

#if 0
	/*poll the slave status
	note: in fact, periodly poll is not needed
	becase each fdplc will report its status in case of its status change
	*/
	static int poll_timer = 0;
	if((poll_timer == 0) || (time_left(poll_timer) < 0)) {
		poll_timer = time_get(500);

		fd_txdat->target = 0;
		fd_txdat->cmd = FDPLC_CMD_POLL;
		fd_txmsg.dlc = 2;
		fdhost_can_send(&fd_txmsg);
	}
#endif
}

void fd_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	memset(fdplc_uuid, 0x00, sizeof(fdplc_uuid));
	memset(fd_rxmsg, 0x00, sizeof(fd_rxmsg));
	memset(&fd_txmsg, 0x00, sizeof(fd_txmsg));
	fd_txmsg.id = CAN_ID_HOST;
	fd_txmsg.dlc = 8;

	fd_gpio_init();
	fd_can->init(&cfg);

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
}

void main()
{
	sys_init();
	//shell_mute(NULL);
	fd_init();
	printf("fdhost v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		fd_update();
	}
}

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"SAFE		poll fd_flag_safe\n"
		"READ id		read (only) fdplc status, id = 1..12, 0=>any one last rx\n"
		"POLL id		read then update fdplc status, id = 0..12, 0=>ALL\n"
		"MOVE id msk img	move vplc cylinders\n"
		"TEST id sql_id	start test of unit 1-12\n"
		"PASS id		debug, identical with cmd 'FDPLC PASS'\n"
		"FAIL id		debug, identical with cmd 'FDPLC FAIL'\n"
		"ENDC id		handshake with TEST_END signal\n"
		"UUID [id]		query/read specific fdplc uuid\n"
		"WDTY			enable test unit wdt function\n"
		"WDTN			disable test unit wdt function\n"
	};

	int i, n, e, v, bytes;
	int id, img, msk, sql;

	if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else if(!strcmp(argv[0], "SAFE")) {
		printf("<%+d\n", fd_flag_safe);
		return 0;
	}
	else if(!strcmp(argv[0], "UUTE")) { //bit mask
		printf("<%+d\n", fd_flag_uute);
		return 0;
	}
	else if(!strcmp(argv[0], "READ")) {
		bytes = 0;
		char hex[64];

		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);
			if((n == 1) && (id >= 0) && (id <= 12)) {
				__disable_irq();
				for(i = 0; i < fd_rxmsg[id].dlc; i ++) {
					v = fd_rxmsg[id].data[i];
					bytes += sprintf(&hex[bytes], "%02x", v & 0xff);
				}
				__enable_irq();

				bytes = fd_rxmsg[id].dlc;
			}
		}
		else { //print status
			msk = 0;
			__disable_irq();
			for(i = 0; i <= NPLC; i ++) {
				bytes += sprintf(&hex[bytes], "%d,", fd_rxmsg[i].dlc);
				if(fd_rxmsg[i].dlc > 0) {
					msk |= 1 << i;
				}
			}
			__enable_irq();

			printf("<%+d, %s\n", msk, hex);
			return 0;
		}

		const char *echo = (bytes > 0) ? hex : "NO MSG";
		printf("<%+d, %s\n", bytes, echo);
		return 0;
	}
	else if(!strcmp(argv[0], "POLL")) {
		e = -1;
		id = 0;
		n = 1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);
		}

		if((n == 1) && (id >= 0) && (id <= 12)) {
			//send poll req
			fd_txdat->target = id;
			fd_txdat->cmd = FDPLC_CMD_POLL;
			fd_txmsg.dlc = 2;
			e = fdhost_can_send(&fd_txmsg);
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "MOVE")) {
		e = -1;
		if(argc > 3) {
			n = sscanf(argv[1], "%d", &id);
			if((argv[2][1] == 'x') || (argv[2][1] == 'X')) n += sscanf(argv[2], "%x", &msk);
			else n += sscanf(argv[2], "%d", &msk);
			if((argv[3][1] == 'x') || (argv[3][1] == 'X')) n += sscanf(argv[3], "%x", &img);
			else n += sscanf(argv[3], "%d", &img);

			if((n == 3) && (id >= 0) && (id <= 12)) { //0=> all move
				fd_txdat->target = id;
				fd_txdat->cmd = FDPLC_CMD_MOVE;
				fd_txdat->move.img = (img >> 18) & 0x03ff; //10 cyls in total
				fd_txdat->move.msk = (msk >> 18) & 0x03ff;
				fd_txmsg.dlc = 8;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "TEST")) {
		e = -1;
		if(argc > 2) {
			n = sscanf(argv[1], "%d", &id);
			if((argv[2][1] == 'x') || (argv[2][1] == 'X')) n += sscanf(argv[2], "%x", &sql);
			else n += sscanf(argv[2], "%d", &sql);

			if((n == 2) && (id >= 1) && (id <= 12)) {
				fd_txdat->target = id;
				fd_txdat->cmd = FDPLC_CMD_TEST;
				fd_txdat->test.sql_id = sql;
				fd_txmsg.dlc = 8;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "PASS")) {
		e = -1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);

			if((n == 1) && (id >= 1) && (id <= 12)) {
				fd_txdat->target = id;
				fd_txdat->cmd = FDPLC_CMD_PASS;
				fd_txmsg.dlc = 2;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "FAIL")) {
		e = -1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);

			if((n == 1) && (id >= 1) && (id <= 12)) {
				fd_txdat->target = id;
				fd_txdat->cmd = FDPLC_CMD_FAIL;
				fd_txmsg.dlc = 2;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "ENDC")) {
		e = -1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);

			if((n == 1) && (id >= 1) && (id <= 12)) {
				fd_txdat->target = id;
				fd_txdat->cmd = FDPLC_CMD_ENDC;
				fd_txmsg.dlc = 2;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, Ulicar Technology, fdhost V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "UUID")) {
		if(argc > 1) {
			id = atoi(argv[1]);
			if((id > 0) && (id <= NPLC)) printf("<%+d, fdplc#%d\n\r", fdplc_uuid[id], id);
			else printf("<%+d, fdplc#%d\n\r", -1, id);
		}
		else { //query all the test unit
			fd_txdat->target = 0;
			fd_txdat->cmd = FDPLC_CMD_UUID;
			fd_txmsg.dlc = 2;
			e = fdhost_can_send(&fd_txmsg);
			printf("<+0, No Error\n\r");
		}
		return 0;
	}
	else if(!strcmp(argv[0], "WDTY")) {
		fd_txdat->target = 0;
		fd_txdat->cmd = FDPLC_CMD_WDTY;
		fd_txmsg.dlc = 2;
		e = fdhost_can_send(&fd_txmsg);
		printf("<%+d, WDTY\n\r", e);
		return 0;
	}
	else if(!strcmp(argv[0], "WDTN")) {
		fd_txdat->target = 0;
		fd_txdat->cmd = FDPLC_CMD_WDTN;
		fd_txmsg.dlc = 2;
		e = fdhost_can_send(&fd_txmsg);
		printf("<%+d, WDTN\n\r", e);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
