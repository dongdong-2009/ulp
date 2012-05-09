/*
* mcamos communication protocol routine
*
* miaofng@2010 initial version
*/

#include "config.h"
#include "err.h"
#include "priv/mcamos.h"
#include "can.h"
#include "ulp_time.h"
#include <string.h>
#include "ulp/sys.h"

//#define MCAMOS_DEBUG

struct mcamos_s mcamos, mcamos_def;

#define MCAMOS_MSG_1_STD_ID mcamos.id_cmd
#define MCAMOS_MSG_2_STD_ID mcamos.id_dat

int mcamos_init_ex(const struct mcamos_s *m)
{
	int ret = 0;
	can_cfg_t cfg = CAN_CFG_DEF;
	struct mcamos_s mcamos_old, mcamos_new;

	//fill mcamos_new
	memcpy(&mcamos_new, &mcamos_def, sizeof(struct mcamos_s));
	if(m != NULL) {
		memcpy(&mcamos_new, m, sizeof(struct mcamos_s));
	}

	if(mcamos_new.can == NULL)
		return -1;

	//default settings
	mcamos_new.baud = (mcamos_new.baud == 0) ? MCAMOS_BAUD : mcamos_new.baud;
	mcamos_new.id_cmd = (mcamos_new.id_cmd == 0) ? MCAMOS_MSG_CMD_ID : mcamos_new.id_cmd;
	mcamos_new.id_dat = (mcamos_new.id_dat == 0) ? MCAMOS_MSG_DAT_ID : mcamos_new.id_dat;
	mcamos_new.timeout = (mcamos_new.timeout == 0) ? MCAMOS_TIMEOUT : mcamos_new.timeout;

	//hw re-init necessary?
	if(!memcmp(&mcamos_new, &mcamos, sizeof(struct mcamos_s)))
		return 0;

	memcpy(&mcamos_old, &mcamos, sizeof(struct mcamos_s));
	memcpy(&mcamos, &mcamos_new, sizeof(struct mcamos_s));

	//can port init?
	if((mcamos_new.can != mcamos_old.can) || (mcamos_new.baud != mcamos_old.baud)) {
		cfg.baud = mcamos_new.baud;
		ret += mcamos_new. can ->init(&cfg);
	}

#if 0
	//can filter init
	can_filter_t filter = {
		.id = mcamos_new.id_dat,
		.mask = 0xffff,
		.flag = 0,
	};
	ret += can -> filt(&filter, 1);
#endif
	return ret;
}

int mcamos_init(const can_bus_t *can, int baud)
{
	mcamos_def.can = can;
	mcamos_def.baud = baud;
	mcamos_def.id_cmd = MCAMOS_MSG_CMD_ID;
	mcamos_def.id_dat = MCAMOS_MSG_DAT_ID;
	return mcamos_init_ex(NULL);
}

int mcamos_dnload(const can_bus_t *can, int addr, const char *buf, int n, int timeout)
{
	int ret;
	can_msg_t msg;
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *)msg.data;
	can = (can == NULL) ? mcamos.can : can;
	timeout = (timeout < 0) ? mcamos.timeout : timeout;
	time_t deadline = time_get(timeout);

	//step1, send command
	cmd -> byAddress[0] = (char) (addr >> 24);
	cmd -> byAddress[1] = (char) (addr >> 16);
	cmd -> byAddress[2] = (char) (addr >> 8);
	cmd -> byAddress[3] = (char) (addr);
	cmd -> byByteCnt[0] = (char) (n >> 8);
	cmd -> byByteCnt[1] = (char) (n);
	cmd -> byDirection = MCAMOS_DOWNLOAD;
	cmd -> byExecute = MCAMOS_NOEXECUTE;

	msg.id = MCAMOS_MSG_1_STD_ID;
	msg.flag = 0;
	msg.dlc = sizeof(struct mcamos_cmd_s);

	can ->flush();
	do {
		ret = can -> send(&msg);
		if(time_left(deadline) < 0)
			return -ERR_TIMEOUT;
	} while(ret);

	//step2, send data
	msg.id = MCAMOS_MSG_2_STD_ID;
	msg.flag = 0;

	while (n > 0) {
		msg.dlc = (n > 8) ? 8 : n;
		memcpy(msg.data, buf, msg.dlc);
		buf += msg.dlc;
		n -= msg.dlc;
		do {
			ret = can -> send(&msg);
			if(time_left(deadline) < 0)
				return -ERR_TIMEOUT;
		} while(ret);
	}

	return ret;
}

int mcamos_upload(const can_bus_t *can, int addr, char *buf, int n, int timeout)
{
	int ret, bytes;
	can_msg_t msg;
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *)msg.data;
	can = (can == NULL) ? mcamos.can : can;
	timeout = (timeout < 0) ? mcamos.timeout : timeout;
	time_t deadline = time_get(timeout);

	//step1, send command
	cmd -> byAddress[0] = (char) (addr >> 24);
	cmd -> byAddress[1] = (char) (addr >> 16);
	cmd -> byAddress[2] = (char) (addr >> 8);
	cmd -> byAddress[3] = (char) (addr);
	cmd -> byByteCnt[0] = (char) (n >> 8);
	cmd -> byByteCnt[1] = (char) (n);
	cmd -> byDirection = MCAMOS_UPLOAD;
	cmd -> byExecute = MCAMOS_NOEXECUTE;

	msg.id = MCAMOS_MSG_1_STD_ID;
	msg.flag = 0;
	msg.dlc = sizeof(struct mcamos_cmd_s);

	can ->flush();
	do {
		ret = can -> send(&msg);
		if(time_left(deadline) < 0)
			return -ERR_TIMEOUT;
	} while(ret);

	//step2, recv data
	while (n > 0) {
		do {
			ret = can -> recv(&msg);
			if(time_left(deadline) < 0)
				return -ERR_TIMEOUT;
		} while(ret);
		if(msg.id == MCAMOS_MSG_2_STD_ID) {
			bytes = (msg.dlc < n) ? msg.dlc : n;
			memcpy(buf, msg.data, bytes);
			buf += bytes;
			n -= bytes;
		}
		else {
			//assert(1 == 0); //dut in factory mode already???
#ifdef MCAMOS_DEBUG
			can_msg_print(&msg, "...strange can frame\n");
#endif
		}
	}

	return ret;
}

int mcamos_execute(const can_bus_t *can, int addr, int timeout)
{
	int ret;
	can_msg_t msg;
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *) msg.data;
	can = (can == NULL) ? mcamos.can : can;
	timeout = (timeout < 0) ? mcamos.timeout : timeout;
	time_t deadline = time_get(timeout);

	cmd -> byAddress[0] = (char) (addr >> 24);
	cmd -> byAddress[1] = (char) (addr >> 16);
	cmd -> byAddress[2] = (char) (addr >> 8);
	cmd -> byAddress[3] = (char) (addr);
	cmd -> byByteCnt[0] = 0;
	cmd -> byByteCnt[1] = 0;
	cmd -> byDirection = MCAMOS_DOWNLOAD;
	cmd -> byExecute = MCAMOS_EXECUTE;

	msg.id = MCAMOS_MSG_1_STD_ID;
	msg.flag = 0;
	msg.dlc = sizeof(struct mcamos_cmd_s);

	can -> flush();
	do {
		ret = can -> send(&msg);
		if(time_left(deadline) < 0)
			return -ERR_TIMEOUT;
	} while(ret);

	return ret;
}

int mcamos_srv_init(mcamos_srv_t *psrv)
{
	int ret;
	can_cfg_t cfg = CAN_CFG_DEF;
	can_filter_t filter[] = {
		{
			.id = psrv->id_cmd,
			.mask = 0xffff,
			.flag = 0,
		},

		{
			.id = psrv->id_dat,
			.mask = 0xffff,
			.flag = 0,
		},
	};

	psrv->baud = (psrv->baud == 0) ? MCAMOS_BAUD : psrv->baud;
	psrv->timeout = (psrv->timeout == 0) ? 10 : psrv->timeout;
	psrv->inbox_addr = (psrv->inbox_addr == 0) ? MCAMOS_INBOX_ADDR : psrv->inbox_addr;
	psrv->outbox_addr = (psrv->outbox_addr == 0) ? MCAMOS_OUTBOX_ADDR : psrv->outbox_addr;

	cfg.baud = psrv->baud;
	ret = psrv->can ->init(&cfg);
	ret += psrv->can ->filt(filter, 2);
	memset(psrv->inbox, 0, 64);
	memset(psrv->outbox, 0, 64);
	return ret;
}

int mcamos_srv_update(mcamos_srv_t * psrv)
{
	int ret, bytes, i, n;
	unsigned addr;
	char dir;
	can_msg_t msg;
	time_t deadline;
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *) msg.data;

	ret = psrv->can->recv(&msg);
	if(ret)
		return 0;

	//can frame received
	if(msg.id != psrv->id_cmd) {
		//strange can frame, ignore
		return -1;
	}

	//process mcamos cmd
	addr = cmd ->byAddress[0];
	addr <<= 8;
	addr |= cmd->byAddress[1];
	addr <<= 8;
	addr |= cmd->byAddress[2];
	addr <<= 8;
	addr |= cmd->byAddress[3];
	addr = ((addr & 0xffffff00) == psrv->inbox_addr) ? ((unsigned) (psrv->inbox) + (addr & 0xff)) : addr;
	addr = ((addr & 0xffffff00) == psrv->outbox_addr) ? ((unsigned) (psrv ->outbox) + (addr & 0xff)) : addr;

	bytes = cmd ->byByteCnt[0];
	bytes <<= 8;
	bytes |= cmd ->byByteCnt[1];

	dir = cmd ->byDirection;

	//try to tx/rx data to/from tester
	deadline = time_get(psrv ->timeout);
	for( i = 0; i < bytes; ) {
		//tester write?
		if(dir == MCAMOS_DOWNLOAD) {
			do {
				ret = psrv ->can ->recv(&msg);
				if(time_left(deadline) < 0)
					return -ERR_TIMEOUT;
			} while(ret);

			//store data
			if(msg.id != psrv->id_dat) {
				return -1;
			}

			memcpy((void *)addr, msg.data, msg.dlc);
			i += msg.dlc;
			addr += msg.dlc;
		}

		//tester read
		else {
			n = bytes - i;
			n = (n > 8) ? 8 : n;
			memcpy(msg.data, (void *)addr, n);
			msg.id = psrv->id_dat;
			msg.dlc = n;
			do {
				ret = psrv ->can ->send(&msg);
				if(time_left(deadline) < 0)
					return -ERR_TIMEOUT;
			} while(ret);

			addr += n;
			i += n;
		}
	}

	return 0;
}

#include "shell/cmd.h"
int cmd_mcamos_func(int argc, char *argv[])
{
	struct mcamos_s m;
	int addr, n, i, v, j;
	char s[8];

	switch(argv[1][0]) {
	case 'i':
		m.can = &can1;
		m.baud = 500000;
		m.id_cmd = 0x7ee;
		if(argc == 3) {
			sscanf(argv[2], "0x%x", &(m.id_cmd));
		}
		m.id_dat = m.id_cmd + 1;
		mcamos_init_ex(&m);
		printf("mcamos.can = can1\n");
		printf("mcamos.baud = %d\n", m.baud);
		printf("mcamos.id_cmd = 0x%x\n", m.id_cmd);
		printf("mcamos.id_dat = 0x%x\n", m.id_dat);
		return 0;

	case 'd':
		if(argc >= 3) {
			sscanf(argv[2], "0x%x", &addr);
			if(argc >= 4) {
				n = strlen(argv[3]);
				for(i = 0, j = 0; n - i >= 2; i += 2, j++) {
					s[0] = argv[3][i];
					s[1] = argv[3][i + 1];
					s[3] = 0;
					sscanf(s, "%x", &v);
					argv[3][j] = (char) v;

					if(j % 8 == 0) { //line head
						printf("0x%08x:	", addr + j);
					}

					if(j % 8 == 7 || j == (n / 2 - 1)) {
						printf("%02x\n", v);
					}
					else if(j % 8 == 3) {
						printf("%02x..", v);
					}
					else {
						printf("%02x  ", v);
					}
				}
				v = mcamos_dnload(mcamos.can, addr, argv[3], j, 10);
				if(v)
					printf("  ...fail\n");
				return 0;
			}
		}
		break;
	case 'u':
		if(argc >= 3) {
			sscanf(argv[2], "0x%x", &addr);
			n = 8;
			if(argc >= 4) {
				sscanf(argv[3], "%d", &n);
			}

			for(i = 0; i < n; i += 8) {
				v = mcamos_upload(mcamos.can, addr + i, s, 8, 10);
				printf("0x%08x:	", addr + i);
				if(!v) {
					for(j = 0; j < 8; j ++) {
						v = (unsigned char) s[j];
						printf("%02x", v);
						if(j != 3)
							printf("  ");
						else
							printf("..");
					}
					printf("\n");
				}
				else {
					printf("...fail\n");
					break;
				}
			}
			return 0;
		}
		break;
	default:
		break;
	}

	printf(
		"mcamos init id_cmd(0x7ee?0x5e0)	mcamos init\n"
		"mcamos dnload 0x0f000000 HHHHHHHHHHH\n"
		"mcamos upload 0x0f000100 N	read N bytes\n"
	);
	return 0;
}

const cmd_t cmd_mcamos = {"mcamos", cmd_mcamos_func, "mcamos cmds"};
DECLARE_SHELL_CMD(cmd_mcamos)

