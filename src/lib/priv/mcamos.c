/*
* mcamos communication protocol routine
*
* miaofng@2010 initial version
*/

#include "config.h"
#include "err.h"
#include "priv/mcamos.h"
#include "can.h"
#include "time.h"
#include <string.h>
#include "debug.h"

int mcamos_init(const can_bus_t *can, int baud)
{
	int ret;
	can_cfg_t cfg = CAN_CFG_DEF;
	can_filter_t filter = {
		.id = MCAMOS_MSG_2_STD_ID,
		.mask = 0xffff,
		.flag = 0,
	};

	cfg.baud = baud;
	ret = can -> init(&cfg);
	ret += can -> filt(&filter, 1);
	return ret;
}

int mcamos_dnload(const can_bus_t *can, int addr, const char *buf, int n, int timeout)
{
	int ret;
	can_msg_t msg;
	time_t deadline = time_get(timeout);
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *)msg.data;

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
	time_t deadline = time_get(timeout);
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *)msg.data;

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
			can_msg_print(&msg, "...strange can frame\n");
		}
	}

	return ret;
}

int mcamos_execute(const can_bus_t *can, int addr, int timeout)
{
	int ret;
	can_msg_t msg;
	time_t deadline = time_get(timeout);
	struct mcamos_cmd_s *cmd = (struct mcamos_cmd_s *) msg.data;

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

	do {
		ret = can -> send(&msg);
		if(time_left(deadline) < 0)
			return -ERR_TIMEOUT;
	} while(ret);

	return ret;
}
