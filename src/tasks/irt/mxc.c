/*
*
*  miaofng@2014-6-6   separate mxc related operations from irc main routine
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "stm32f10x.h"
#include "can.h"
#include "bsp.h"
#include "mxc.h"
#include <string.h>
#include "shell/cmd.h"

static const can_bus_t *mxc_bus = &can1;
static can_msg_t mxc_msg;

int mxc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	mxc_bus->init(&cfg);

	/*tricky, to bring slots back from deadlock on waiting le signal*/
	le_set(1);
	mdelay(1);
	le_set(0);
	return 0;
}

int mxc_send(const can_msg_t *msg)
{
	int ecode = -IRT_E_CAN;
	time_t deadline = time_get(IRC_CAN_MS);
	mxc_bus->flush();
	if(!mxc_bus->send(msg)) { //wait until message are sent
		while(time_left(deadline) > 0) {
			if(mxc_bus->poll(CAN_POLL_TBUF) == 0) {
				ecode = 0;
				break;
			}
		}
	}
	return ecode;
}

int mxc_latch(void)
{
	int ecode = -IRT_E_SLOT;
	time_t deadline = time_get(IRC_RLY_MS);
	time_t suspend = time_get(IRC_UPD_MS);

	le_set(1);
	while(time_left(deadline) > 0) {
		if(le_get() > 0) { //ok? break
			le_set(0);
			ecode = 0;
			break;
		}

		//update if waiting too long ... to avoid system suspend
		if(time_left(suspend) < 0) {
			sys_update();
		}
	}

	return ecode;
}

int mxc_mode(int mode)
{
	int ecode = 0;
	int bus, lne;

	sys_assert((mode >= IRC_MODE_HVR) && (mode < IRC_MODE_OFF));

	//step 1, ibus?ebus + iline?eline
	bus = (mode < IRC_MODE_PRB) ? MXC_ALL_IBUS : MXC_ALL_EBUS;
	lne = (mode > IRC_MODE_PRB) ? MXC_ALL_ILNE : MXC_ALL_ELNE;

	//step 2, fill slot config message
	mxc_cfg_msg_t *cfg = (mxc_cfg_msg_t *) mxc_msg.data;
	cfg->cmd = MXC_CMD_CFG;
	cfg->ms = IRC_RLY_MS;
	cfg->slot = MXC_ALL_SLOT;
	cfg->bus = bus;
	cfg->line = lne;

	//step 3, send ...
	mxc_msg.id = CAN_ID_CFG;
	mxc_msg.dlc = sizeof(mxc_cfg_msg_t);
	ecode = mxc_send(&mxc_msg);

	//step 4, send le signal
	if(!ecode) {
		ecode = mxc_latch();
	}
	return ecode;
}

int mxc_ping(int slot)
{
	mxc_echo_msg_t *echo = (mxc_echo_msg_t *) mxc_msg.data;
	mxc_cfg_msg_t *cfg = (mxc_cfg_msg_t *) mxc_msg.data;

	cfg->cmd = MXC_CMD_PING;
	cfg->slot = (unsigned char) slot;
	mxc_msg.id = CAN_ID_CFG;
	mxc_msg.dlc = sizeof(mxc_cfg_msg_t);
	int ecode = mxc_send(&mxc_msg);
	if(!ecode) {
		ecode = -IRT_E_NA;
		time_t deadline = time_get(IRC_ECO_MS);
		while(time_left(deadline) > 0) {
			if(mxc_bus->recv(&mxc_msg) == 0) {
				int match = (mxc_msg.id == CAN_ID_CFG);
				match = match && (echo->cmd == MXC_CMD_ECHO);
				match = match && (echo->slot == slot);
				if(match) {
					ecode = echo->ecode;
				}
			}
		}
	}
	return ecode;
}

int mxc_scan(int min, int max)
{
	int ecode = 0;
	int slots = 0;
	for(int slot = min; (slot <= max) || (!max && !ecode); slot ++) {
		ecode = mxc_ping(slot);
		slots += (ecode != -IRT_E_NA) ? 1 : 0;
		printf("slot %02d: ", slot); err_print(ecode);
	}
	printf("Total %d cards\n", slots);
	return slots;
}

static int cmd_mxc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mxc ping <slot>		ping specified slot\n"
		"mxc scan [min] [max]	abort until fail if max is not given\n"
		"mxc help\n"
	};

	if((argc > 1) && !strcmp(argv[1], "ping")) {
		int slot = (argc > 2) ? atoi(argv[2]) : 0;
		int ecode = mxc_ping(slot);
		printf("slot = %02d, ", slot); err_print(ecode);
	}
	else if((argc > 1) && !strcmp(argv[1], "scan")) {
		int ecode = 0;
		int min = (argc > 2) ? atoi(argv[2]) : 0;
		int max = (argc > 2) ? atoi(argv[3]) : 0;
		mxc_scan(min, max);
	}
	else if((argc > 0) && !strcmp(argv[1], "help")) {
		printf("%s", usage);
	}

	return 0;
}
const cmd_t cmd_mxc = {"mxc", cmd_mxc_func, "mxc related debug cmds"};
DECLARE_SHELL_CMD(cmd_mxc)
