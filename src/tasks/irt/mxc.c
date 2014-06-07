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

static const can_bus_t *mxc_bus = &can1;
static can_msg_t mxc_msg;

int mxc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	mxc_bus->init(&cfg);
	return 0;
}

int mxc_send(const can_msg_t *msg)
{
	int ecode = -IRT_E_CAN;
	time_t deadline = time_get(IRC_CAN_MS);
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
