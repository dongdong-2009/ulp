/*
*
* miaofng@2017-07-16 routine for vw
* 1, vw system includes vw and vwhost
* 2, all cylinders are controled by vwhost, vw acts like a plc io extend module
* 3, handshake between test program and vw: rdy4tst/test_end/test_ng
* 4, vw can not request test unit ID
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
#define CAN_ID_HOST 0x100 //0x101..120 is test unit

enum {
	VW_CMD_POLL, //host: cmd -> slave: response status
	VW_CMD_SCAN, //host: cmd+rsel+csel
	VW_CMD_MOVE, //host: cmd
	VW_CMD_TEST, //host: cmd + mysql_record_id
	VW_CMD_PASS, //host: cmd, identical with cmd "TEST PASS", only for debug purpose
	VW_CMD_FAIL, //host: cmd, identical with cmd "TEST FAIL", only for debug purpose
	VW_CMD_ENDC, //host: cmd, clear test_end request, indicates all cyl has been released
};

typedef struct {
	char target; //0: all slave
	char cmd; //assign, move, (get)status, ...
	char img; //used by move
	char msk; //used by move
	int sql_id; //used by test, test unit query the mysql of barcode through it
} txdat_t;

typedef struct {
	char id; //test unit id, assign by host, range: 1-18
	char reserved; //
	char test_end;
	char test_ng;
	int sensors;
} rxdat_t;

static const can_bus_t *vw_can = &can1;
static can_msg_t vw_rxmsg[10];
static can_msg_t vw_txmsg;
static rxdat_t *vw_rxdat = (rxdat_t *) vw_rxmsg[0].data;
static txdat_t *vw_txdat = (txdat_t *) vw_txmsg.data;

void vw_gpio_init(void)
{
	//sensors #00-07
	GPIO_BIND(GPIO_IPU, PC13, GPI#00) //GPI#00
	GPIO_BIND(GPIO_IPU, PC14, GPI#01) //GPI#01
	GPIO_BIND(GPIO_IPU, PC15, GPI#02) //GPI#02
	GPIO_BIND(GPIO_IPU, PA1, GPI#03) //GPI#03

	GPIO_BIND(GPIO_IPU, PA2, GPI#04) //GPI#04
	GPIO_BIND(GPIO_IPU, PA4, GPI#05) //GPI#05
	GPIO_BIND(GPIO_IPU, PA5, GPI#06) //GPI#06
	GPIO_BIND(GPIO_IPU, PA6, GPI#07) //GPI#07

	//sensors #10-17
	GPIO_BIND(GPIO_IPU, PA7, GPI#10) //GPI#10
	GPIO_BIND(GPIO_IPU, PC4, GPI#11) //GPI#11
	GPIO_BIND(GPIO_IPU, PC5, GPI#12) //GPI#12
	GPIO_BIND(GPIO_IPU, PB0, GPI#13) //GPI#13

	GPIO_BIND(GPIO_IPU, PB1, GPI#14) //GPI#14
	GPIO_BIND(GPIO_IPU, PB2, GPI#15) //GPI#15
	GPIO_BIND(GPIO_IPU, PB10, GPI#16) //GPI#16
	GPIO_BIND(GPIO_IPU, PB11, GPI#17) //GPI#17

	//valve ctrl
	GPIO_BIND(GPIO_PP0, PB12, RSEL0) //GPO#00
	GPIO_BIND(GPIO_PP0, PB13, RSEL1) //GPO#01
	GPIO_BIND(GPIO_PP0, PB14, RSEL2) //GPO#02
	GPIO_BIND(GPIO_PP0, PB15, GPO#3) //GPO#03

	GPIO_BIND(GPIO_PP0, PC6, CSEL0) //GPO#04
	GPIO_BIND(GPIO_PP0, PC7, CSEL1) //GPO#05
	GPIO_BIND(GPIO_PP0, PC8, CSEL2) //GPO#06
	GPIO_BIND(GPIO_PP0, PC9, GPO#7) //GPO#07

	//misc
	GPIO_BIND(GPIO_IPU, PC12, ESTOP#) //RSEL
	GPIO_BIND(GPIO_IPU, PC11, RST#) //CSEL

	//GPIO_BIND(GPIO_PP0, PA8, MPC_ON)

	GPIO_BIND(GPIO_AIN, PC2, VMPC)
	GPIO_BIND(GPIO_AIN, PC3, VBST)
}

/*id range: [1, 9]*/
int vw_assign(int idx)
{
	sys_assert((idx >= 0) && (idx < 9));
	const int slaves[] = { //operator view
		0x3011, 0x6012, 0x9014,
		0x2021, 0x5022, 0x8024,
		0x1041, 0x4042, 0x7044,
	};

	//convert idx to id
	int id = slaves[idx] >> 12;
	id &= 0x0f;

	static int last_idx = -1;
	if(last_idx >= 0) {
		//csel/rsel signals need stable time
		vw_txdat->target = id;
		vw_txdat->cmd = VW_CMD_SCAN;
		vw_can->send(&vw_txmsg);

		last_idx = -1;
		return 0; //id been assigned
	}

	char rsel = (slaves[idx] >> 4) & 0x0f;
	char csel = (slaves[idx] >> 0) & 0x0f;
	gpio_set("RSEL0", rsel & 0x01);
	gpio_set("RSEL1", rsel & 0x02);
	gpio_set("RSEL2", rsel & 0x04);
	gpio_set("CSEL0", csel & 0x01);
	gpio_set("CSEL1", csel & 0x02);
	gpio_set("CSEL2", csel & 0x04);
	last_idx = idx;
	return id;
}

void vw_update(void)
{
	//so boring .. assign the vwplc id :(
	static int idx_scan = 0;
	static int idx_scan_timer = 0;

	if((idx_scan_timer == 0) || (time_left(idx_scan_timer) < 0)) {
		idx_scan_timer = time_get(100);
		int idx = vw_assign(idx_scan);
		idx_scan += (idx == 0) ? 1 : 0;
		idx_scan = (idx_scan > 8) ? 0 : idx_scan;
	}

	//poll result?
	while(1) {
		int e = vw_can->recv(&vw_rxmsg[0]);
		if(e) break;

		int id = vw_rxdat->id;
		memcpy(&vw_rxmsg[id], &vw_rxmsg[0], sizeof(can_msg_t));
	}
}

void vw_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	memset(&vw_rxmsg, 0x00, sizeof(vw_rxmsg));
	memset(&vw_txmsg, 0x00, sizeof(vw_txmsg));
	vw_txmsg.id = CAN_ID_HOST;
	vw_txmsg.dlc = 8;

	vw_gpio_init();
	vw_can->init(&cfg);
}

void main()
{
	sys_init();
	shell_mute(NULL);
	vw_init();
	printf("vwhost v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		vw_update();
	}
}

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"READ id		read (only) vwplc status, id = 1..9, 0=>any one last rx\n"
		"POLL id		read then update vwplc status, id = 0..9, 0=>ALL\n"
		"MOVE id msk img	move vplc 1-9 cylinders\n"
		"TEST id sql_id	start test of unit 1-9\n"
		"PASS id		debug, identical with cmd 'VWPLC PASS'\n"
		"FAIL id		debug, identical with cmd 'VWPLC FAIL'\n"
		"ENDC id		handshake with TEST_END signal\n"
	};

	int i, n, e, v, bytes;
	int id, img, msk, sql;

	if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else if(!strcmp(argv[0], "READ")) {
		bytes = 0;
		char hex[64];

		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);
			if((n == 1) && (id >= 0) && (id <= 9)) {
				for(i = 0; i < vw_rxmsg[id].dlc; i ++) {
					v = vw_rxmsg[id].data[i];
					bytes += sprintf(&hex[bytes], "%02x", v & 0xff);
				}

				bytes = vw_rxmsg[id].dlc;
			}

		}
		else { //print status
			msk = 0;
			for(i = 0; i < 10; i ++) {
				bytes += sprintf(&hex[bytes], "%d,", vw_rxmsg[i].dlc);
				if(vw_rxmsg[i].dlc > 0) {
					msk |= 1 << i;
				}
			}

			printf("<%+d, %s\n", msk, hex);
			return 0;
		}

		const char *echo = (bytes > 0) ? hex : "NO MSG";
		printf("<%+d, %s\n", bytes, echo);
		return 0;
	}
	else if(!strcmp(argv[0], "POLL")) {
		e = -1;
		bytes = 0;
		char hex[64];

		id = 0;
		n = 1;
		if(argc > 1) {
			n = sscanf(argv[1], "%d", &id);
		}

		if((n == 1) && (id >= 0) && (id <= 9)) {
			//prepare for read back
			for(i = 0; i < vw_rxmsg[id].dlc; i ++) {
				v = vw_rxmsg[id].data[i];
				bytes += sprintf(&hex[bytes], "%02x", v & 0xff);
			}
			bytes = vw_rxmsg[id].dlc;

			//clear for new coming msg
			vw_rxmsg[id].dlc = 0;
			if(id == 0) {
				for(i = 1; i < 9; i ++) {
					vw_rxmsg[i].dlc = 0;
				}
			}

			//send poll req
			vw_txdat->target = id;
			vw_txdat->cmd = VW_CMD_POLL;
			e = vw_can->send(&vw_txmsg);
		}

		const char *echo = e ? "Error" : hex;
		bytes = e ? -1 : bytes;

		printf("<%+d, %s\n", bytes, echo);
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

			if((n == 3) && (id >= 1) && (id <= 9)) {
				vw_txdat->target = id;
				vw_txdat->cmd = VW_CMD_MOVE;
				vw_txdat->img = (img >> 16) & 0xff;
				vw_txdat->msk = (msk >> 16) & 0xff;
				e = vw_can->send(&vw_txmsg);
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

			if((n == 2) && (id >= 1) && (id <= 9)) {
				vw_txdat->target = id;
				vw_txdat->cmd = VW_CMD_TEST;
				vw_txdat->sql_id = sql;
				e = vw_can->send(&vw_txmsg);
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

			if((n == 1) && (id >= 1) && (id <= 9)) {
				vw_txdat->target = id;
				vw_txdat->cmd = VW_CMD_PASS;
				e = vw_can->send(&vw_txmsg);
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

			if((n == 1) && (id >= 1) && (id <= 9)) {
				vw_txdat->target = id;
				vw_txdat->cmd = VW_CMD_FAIL;
				e = vw_can->send(&vw_txmsg);
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

			if((n == 1) && (id >= 1) && (id <= 9)) {
				vw_txdat->target = id;
				vw_txdat->cmd = VW_CMD_ENDC;
				e = vw_can->send(&vw_txmsg);
			}
		}

		const char *emsg = e ? "Error" : "OK";
		printf("<%+d, %s\n", e, emsg);
		return 0;
	}
	else if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, Ulicar Technology, vwhost V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
