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
#include "fdplc.h"

static int fd_flag_safe = -1; //bitor of all fdpls's sm-
static int fd_flag_uute = -1; //bitor of all fdpls's uute
static int fdplc_uuid[FD_NPLC + 1]; //all fdplc's uuid

static const can_bus_t *fd_can = &can1;
static can_msg_t fd_rxmsg[FD_NPLC + 1];
static can_msg_t fd_txmsg;
static fdrpt_t *fd_rxdat = (fdrpt_t *) fd_rxmsg[0].data;
static fdcmd_t *fd_txdat = (fdcmd_t *) fd_txmsg.data;
static int fd_hgpio_safe = -1;

static int fdhost_can_send(can_msg_t *msg)
{
	const fdcmd_t *txdat = (const fdcmd_t *) msg->data;
	fdcmd_type_t cmd_type = (fdcmd_type_t) txdat->cmd;
	switch(cmd_type) {
	case FDCMD_SCAN:
	case FDCMD_POLL:
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
	for(int i = 0; i <= FD_NPLC; i ++) {
		if(fd_rxmsg[i].id <= 0) fd_rxmsg[i].dlc = 0;
		else {
			fd_rxmsg[i].id --;
			fdrpt_t *rxdat = (fdrpt_t *) fd_rxmsg[i].data;
			if(rxdat->sensors & FDRPT_MASK_UUTE) {
				uute |= 1 << i;
			}
		}
	}
	fd_flag_uute = uute;
}

static void fdhost_can_handler(void)
{
	int src = fd_rxdat->src;
	fdcmd_type_t cmd_type = (fdcmd_type_t) fd_rxdat->cmd;
	if(cmd_type == FDCMD_UUID) {
		fdplc_uuid[src] = fd_rxdat->sensors;
		return;
	}

	//can msg id is used as alive detection counter
	fd_rxmsg[0].id = FD_ALIVE_MS;
	memcpy(&fd_rxmsg[src], &fd_rxmsg[0], sizeof(can_msg_t));

	//danger process
	int safe = 1;
	for(int i = 1; i <= FD_NPLC; i ++) {
		if(fd_rxmsg[i].dlc > 0) {
			fdrpt_t *rxdat = (fdrpt_t *) fd_rxmsg[i].data;
			if((rxdat->sensors & FDRPT_MASK_SM_N) == 0) {
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

	GPIO_BIND(GPIO_PP1, PC00, LED_R)
	GPIO_BIND(GPIO_PP1, PC01, LED_G)

	//GPIO_BIND(GPIO_PP0, PA08, MPC_ON)
	GPIO_BIND(GPIO_AIN, PC02, VMPC)
	GPIO_BIND(GPIO_AIN, PC03, VBST)
}

/*idx range: [0, FD_NPLC-1]*/
int fd_assign(int idx)
{
	sys_assert((idx >= 0) && (idx < FD_NPLC));
	const int slaves[] = { //operator view
		0x1011, 0x2012, 0x3014,
		0x4021, 0x5022, 0x6024,
		0x7041, 0x8042, 0x9044,
		0xa081, 0xb082, 0xc084,
	};

	//convert idx to dst
	int dst = slaves[idx] >> 12;
	dst &= 0x0f;

	static int last_idx = -1;
	if(last_idx >= 0) {
		//csel/rsel signals need stable time
		fd_txdat->dst = dst;
		fd_txdat->cmd = FDCMD_SCAN;
		fd_txmsg.dlc = 4;
		fdhost_can_send(&fd_txmsg);

		last_idx = -1;
		return 0; //dst id been assigned
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
	return dst;
}

void fd_update(void)
{
	//so boring .. assign the fdplc id :(
	static int idx_scan = 0;
	static int scan_timer = 0;

	if((scan_timer == 0) || (time_left(scan_timer) < 0)) {
		scan_timer = time_get(FD_SCAN_MS / FD_NPLC / 2);
		int idx = fd_assign(idx_scan);
		idx_scan += (idx == 0) ? 1 : 0;
		idx_scan = (idx_scan >= FD_NPLC) ? 0 : idx_scan;
	}

#if 0
	/*poll the slave status
	note: in fact, periodly poll is not needed
	becase each fdplc will report its status in case of its status change
	*/
	static int poll_timer = 0;
	if((poll_timer == 0) || (time_left(poll_timer) < 0)) {
		poll_timer = time_get(500);

		fd_txdat->dst = 0;
		fd_txdat->cmd = FDCMD_POLL;
		fd_txmsg.dlc = 4;
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
	shell_mute(NULL);
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
		"CNTR id		reset target test unit pushed counter\n"
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
			for(i = 0; i <= FD_NPLC; i ++) {
				int ms_left = 0;
				if(fd_rxmsg[i].dlc > 0) {
					msk |= 1 << i;
					ms_left = fd_rxmsg[i].id;
				}
				bytes += sprintf(&hex[bytes], "%04d,", ms_left);
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
			fd_txdat->dst = id;
			fd_txdat->cmd = FDCMD_POLL;
			fd_txmsg.dlc = 4;
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
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_MOVE;
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
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_TEST;
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
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_PASS;
				fd_txmsg.dlc = 4;
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
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_FAIL;
				fd_txmsg.dlc = 4;
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
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_ENDC;
				fd_txmsg.dlc = 4;
				e = fdhost_can_send(&fd_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "CNTR")) {
		e = -1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);

			if((n == 1) && (id >= 1) && (id <= 12)) {
				fd_txdat->dst = id;
				fd_txdat->cmd = FDCMD_CNTR;
				fd_txmsg.dlc = 4;
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
			if((id > 0) && (id <= FD_NPLC)) printf("<%+d, fdplc#%d\n\r", fdplc_uuid[id], id);
			else printf("<%+d, fdplc#%d\n\r", -1, id);
		}
		else { //query all the test unit
			fd_txdat->dst = 0;
			fd_txdat->cmd = FDCMD_UUID;
			fd_txmsg.dlc = 4;
			e = fdhost_can_send(&fd_txmsg);
			printf("<+0, No Error\n\r");
		}
		return 0;
	}
	else if(!strcmp(argv[0], "WDTY")) {
		fd_txdat->dst = 0;
		fd_txdat->cmd = FDCMD_WDTY;
		fd_txmsg.dlc = 4;
		e = fdhost_can_send(&fd_txmsg);
		printf("<%+d, WDTY\n\r", e);
		return 0;
	}
	else if(!strcmp(argv[0], "WDTN")) {
		fd_txdat->dst = 0;
		fd_txdat->cmd = FDCMD_WDTN;
		fd_txmsg.dlc = 4;
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
