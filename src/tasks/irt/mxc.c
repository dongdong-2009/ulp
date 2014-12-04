/*
*
*  miaofng@2014-6-6   separate mxc related operations from irc main routine
*  mxc failure must be handled by the caller.
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

static LIST_HEAD(mxc_list);
static can_msg_t mxc_msg;
static struct mxc_s mxc_tmp;
static struct mxc_s *mxc_new;
static int mxc_ping_enable;

struct mxc_s *mxc_search(int slot)
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
	mxc_new = NULL;
	mxc_ping_enable = 0;

	/*tricky, to bring slots back from deadlock on waiting le signal*/
	le_set(1);
	mdelay(1);
	le_set(0);
	mdelay(1);
}

/*called by can isr, please don't:
1, call non-re-entrant routines like malloc/...
2, do not share glvar like mxc_msg with main routine
3, do not share hardware like can controller with main routine
*/

static int ecode_slot[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void mxc_can_handler(can_msg_t *msg)
{
	mxc_echo_t *echo = (mxc_echo_t *) msg->data;
	int node = CAN_NODE(msg->id);
	int slot = MXC_SLOT(node);
	if(echo->ecode > 0) {
		ecode_slot[slot] = echo->ecode;
	}

	struct mxc_s *mxc = mxc_search(slot);
	if(mxc != NULL) {
		mxc->ecode = echo->ecode;
		mxc->flag = echo->flag;
		mxc->nlost = 0;
	}
	else { //create new
		if(mxc_new == NULL) {
			mxc_tmp.slot = slot;
			mxc_tmp.ecode = echo->ecode;
			mxc_tmp.flag = echo->flag;
			mxc_tmp.nlost = 0;
			mxc_tmp.timer = time_get(0);
			mxc_new = &mxc_tmp;
		}
	}
}

void mxc_update(void)
{
	struct list_head *pos;
	struct mxc_s *q = NULL;

	if(mxc_new != NULL) {
		struct mxc_s *mxc = sys_malloc(sizeof(struct mxc_s));
		sys_assert(mxc != NULL);
		memcpy(mxc, mxc_new, sizeof(struct mxc_s));
		list_add(&mxc->list, &mxc_list);
		mxc_new = NULL;
	}

	int ping_is_done = 0;
	list_for_each(pos, &mxc_list) {
		q = list_entry(pos, mxc_s, list);
		if(!mxc_ping_enable) {
			if(q->flag & (1 << MXC_INIT)) {
				int ecode = mxc_offline(q->slot);
				/* preset to offline state in advance
				because periodically ping need a long time ..
				in case of offline failure, slot will send req again
				*/
				q->flag &= ~(1 << MXC_INIT);
				printf("offline slot %d, ecode=%d\n", q->slot, q->ecode);
				irc_error(ecode);
			}
		}
		else {
			if(!ping_is_done) { //ping once per-update
				if(time_left(q->timer) < 0) {
					q->timer = time_get(IRC_POL_MS);
					q->nlost += (q->nlost < IRC_POL_NL) ? 1 : 0;
					mxc_ping(q->slot, 0);
					ping_is_done = 1;
				}
			}
		}
	}
}

int mxc_scan(void *image, int type)
{
	struct list_head *pos;
	struct mxc_s *q = NULL;
	int match, nr_of_slots = 0;

	list_for_each(pos, &mxc_list) {
		q = list_entry(pos, mxc_s, list);
		switch(type) {
		case MXC_ALL:
			match = 1;
			break;
		case MXC_GOOD:
			match = (q->ecode == IRT_E_OK);
			match &= (q->nlost < IRC_POL_NL);
			break;
		case MXC_FAIL:
			match = (q->ecode != IRT_E_OK);
			match |= (q->nlost >= IRC_POL_NL);
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
	int ecode = -IRT_E_LATCH_H;
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
	mxc_ping_enable = 1;
	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));
	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_cfg_t *cfg = (mxc_cfg_t *) mxc_msg.data;
	cfg->cmd = MXC_CMD_MODE;
	cfg->ms = IRC_RLY_MS;
	cfg->mode = mode;
	cfg->safelatch = IRC_LATCH_TWICE;

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
	struct mxc_s *mxc = mxc_search(slot);
	if(mxc == NULL) {
		return -IRT_E_SLOT_NA_OR_LOST;
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
			ecode = -IRT_E_SLOT_NA_OR_LOST;
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
		"MXC PING <slot>		ping specified slot\n"
		"MXC SCAN GOOD			list [ALL|GOOD|FAIL|SELF|DCFM] slots\n"
		"MXC HELP\n"
	};

	if((argc > 1) && !strcmp(argv[1], "PING")) {
		int slot = (argc > 2) ? atoi(argv[2]) : 0;
		int ecode = mxc_ping(MXC_NODE(slot), IRC_ECO_MS);
		irc_error_print(ecode);
	}
	else if((argc > 1) && !strcmp(argv[1], "SCAN")) {
		const char *name[] = {
			"ALL", "GOOD", "FAIL", "SELF", "DCFM"
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
	else if((argc > 0) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
	}

	return 0;
}
const cmd_t cmd_mxc = {"MXC", cmd_mxc_func, "mxc related debug cmds"};
DECLARE_SHELL_CMD(cmd_mxc)
