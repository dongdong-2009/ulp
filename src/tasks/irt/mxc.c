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
#include "vm.h"
#include "common/bitops.h"
#include "irc.h"

static can_msg_t mxc_msg;
static LIST_HEAD(mxc_list);

static struct mxc_s *mxc_search(int slot)
{
	struct list_head *pos;
	struct mxc_s *q = NULL;

	list_for_each(pos, &mxc_list) {
		q = list_entry(pos, mxc_s, list);
		if(q->slot == slot) { //found one
			return q;
		}
	}

	return NULL;
}

void mxc_init(void)
{
	/*tricky, to bring slots back from deadlock on waiting le signal*/
	le_set(1);
	mdelay(1);
	le_set(0);
	mdelay(1);
}

void mxc_can_handler(can_msg_t *msg)
{
	mxc_echo_t *echo = (mxc_echo_t *) msg->data;
	int node = CAN_NODE(msg->id);
	int slot = MXC_SLOT(node);

	if(echo->flag & MXC_INIT) {
		mxc_offline(slot);
	}

	struct mxc_s *mxc = mxc_search(slot);
	if(mxc != NULL) {
		mxc->ecode = echo->ecode;
		mxc->flag = echo->flag;
		mxc->timer = time_get(0);
	}
	else {
		//create new
		mxc = sys_malloc(sizeof(mxc_echo_t));
		sys_assert(mxc != NULL);
		mxc->slot = slot;
		mxc->ecode = echo->ecode;
		mxc->flag = echo->flag;
		mxc->timer = time_get(0);
		list_add(&mxc->list, &mxc_list);
	}
}

void mxc_update(void)
{
	struct list_head *pos;
	struct mxc_s *q = NULL;

	list_for_each(pos, &mxc_list) {
		q = list_entry(pos, mxc_s, list);
		if(time_left(q->timer) < -IRC_POL_MS) {
			mxc_ping(q->slot, 0);
			break;
		}
	}
}

int mxc_scan(void *image, int type)
{
	struct list_head *pos;
	struct mxc_s *q = NULL;
	int ms, match, nr_of_slots = 0;

	static const int timeout = (int)(-IRC_POL_MS*NR_OF_SLOT_MAX*1.5);

	list_for_each(pos, &mxc_list) {
		q = list_entry(pos, mxc_s, list);
		ms = time_left(q->timer);
		switch(type) {
		case MXC_ALL:
			match = 1;
			break;
		case MXC_GOOD:
			match = (q->ecode == IRT_E_OK);
			match &= (ms > timeout);
			break;
		case MXC_FAIL:
			match = (q->ecode != IRT_E_OK);
			match |=  (ms <= timeout);
			break;
		case MXC_SELF:
			match = (q->flag & (1 << MXC_SELF));
			break;
		case MXC_DCFM:
			match = (q->flag & (1 << MXC_DCFM));
			break;
		default:
			match = 0;
		}

		if(match) {
			nr_of_slots ++;
			if(image != NULL) {
				bit_set(q->slot, image);
			}
		}
	}
	return nr_of_slots;
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

	irc_error(ecode);
	return ecode;
}

int mxc_mode(int mode)
{
	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));
	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_cfg_t *cfg = (mxc_cfg_t *) mxc_msg.data;
	cfg->cmd = MXC_CMD_MODE;
	cfg->ms = IRC_RLY_MS;
	cfg->mode = mode;

	//step 3, send ...
	mxc_msg.id = CAN_ID_MXC | MXC_ALL;
	mxc_msg.dlc = sizeof(mxc_cfg_t);
	int ecode = irc_send(&mxc_msg);

	//step 4, send le signal
	if(!ecode) {
		ecode = mxc_latch();
	}
	return ecode;
}

int mxc_ping(int slot, int ms)
{
	sys_assert((slot >= 0) && (slot < NR_OF_SLOT_MAX));
	struct mxc_s *mxc = mxc_search(slot);
	if(mxc == NULL) {
		return -IRT_E_NA;
	}

	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_cfg_t *cfg = (mxc_cfg_t *) mxc_msg.data;
	cfg->cmd = MXC_CMD_PING;
	mxc_msg.id = CAN_ID_MXC | MXC_NODE(slot);
	mxc_msg.dlc = sizeof(mxc_cfg_t);
	int ecode = irc_send(&mxc_msg);
	if(!ecode) {
		ecode = mxc->ecode;
		if(ms > 0) {
			ecode = -IRT_E_NA;
			time_t deadline = time_get(ms);
			time_t backup = mxc->timer;
			while(time_left(deadline) > 0) {
				if(backup != mxc->timer) { //irq occurs
					ecode = mxc->ecode;
				}
			}
		}
	}
	return ecode;
}

int mxc_reset(int slot)
{
	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_cfg_t *cfg = (mxc_cfg_t *) mxc_msg.data;

	cfg->cmd = MXC_CMD_RESET;
	mxc_msg.id = CAN_ID_MXC | MXC_NODE(slot);
	mxc_msg.dlc = sizeof(mxc_cfg_t);
	int ecode = irc_send(&mxc_msg);
	return ecode;
}

int mxc_offline(int slot)
{
	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_cfg_t *cfg = (mxc_cfg_t *) mxc_msg.data;

	cfg->cmd = MXC_CMD_OFFLINE;
	mxc_msg.id = CAN_ID_MXC | MXC_NODE(slot);
	mxc_msg.dlc = sizeof(mxc_cfg_t);
	int ecode = irc_send(&mxc_msg);
	return ecode;
}

static int cmd_mxc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mxc ping <slot>		ping specified slot\n"
		"mxc scan good		list [all|good|fail|self|dcfm] slots\n"
		"mxc help\n"
	};

	if((argc > 1) && !strcmp(argv[1], "ping")) {
		int slot = (argc > 2) ? atoi(argv[2]) : 0;
		int ecode = mxc_ping(MXC_NODE(slot), IRC_ECO_MS);
		irc_error_print(ecode);
	}
	else if((argc > 1) && !strcmp(argv[1], "scan")) {
		const char *name[] = {
			"all", "good", "fail", "self", "dcfm"
		};
		const int type_list[] = {
			MXC_ALL, MXC_GOOD, MXC_FAIL, MXC_SELF, MXC_DCFM
		};

		int type = MXC_GOOD;
		if(argc > 2) {
			for(int i = 0; i < sizeof(type_list) / sizeof(int); i ++) {
				if(!strcmp(argv[2], name[i])) {
					type = type_list[i];
					break;
				}
			}
		}

		char image[NR_OF_SLOT_MAX/8];
		memset(image, 0x00, sizeof(image));
		int slots = mxc_scan(image, type);
		printf("<%+d,\"", slots);
		for(int i = 0; i < NR_OF_SLOT_MAX; i ++) {
			if(bit_get(i, image)) {
				printf("%d,", i);
			}
		}
		printf("\"\n\r");
	}
	else if((argc > 0) && !strcmp(argv[1], "help")) {
		printf("%s", usage);
	}

	return 0;
}
const cmd_t cmd_mxc = {"mxc", cmd_mxc_func, "mxc related debug cmds"};
DECLARE_SHELL_CMD(cmd_mxc)
