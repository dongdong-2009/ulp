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
#include "common/circbuf.h"
#include "irc.h"
#include "bsp.h"
#include "dps.h"

static LIST_HEAD(mxc_list);
static can_msg_t mxc_msg;
static struct mxc_s mxc_tmp;
static struct mxc_s *mxc_new;
static int mxc_ping_enable;

/*var for vm_execute*/
static int opcode_type_saved;
static circbuf_t mxc_odata = {.data = NULL};
static char mxc_frame_counter;
static can_msg_t mxc_msg;
static char mxc_swdebug;
static struct {
	unsigned char busy : 1;
	unsigned char hv : 1;
} mxc_flag;

/*trig dmm to do measurement and wait for operation finish*/
static int mxc_measure(void)
{
	int ecode = -IRT_E_DMM;
	time_t deadline, suspend;

	//apply pre-trig delay
	vm_wait(vm_get_measure_delay());

	trig_set(1);
	deadline = time_get(IRC_DMM_MS);
	suspend = time_get(IRC_UPD_MS);
	while(time_left(deadline) > 0) {
		if(trig_get() == 1) { //vmcomp pulsed
			trig_set(0);
			ecode = 0;
			break;
		}

		if(time_left(suspend) < 0) {
			irc_update();
		}
	}

	irc_error(ecode);
	return ecode;
}

static void mxc_emit(int can_id, int do_measure)
{
	memset(&mxc_msg, 0x00, sizeof(mxc_msg));
	mxc_msg.dlc = buf_size(&mxc_odata);
	mxc_msg.id = can_id;
	mxc_msg.id += mxc_frame_counter & 0x0F;
	if(mxc_msg.dlc > 0) {
		mxc_frame_counter = (can_id == CAN_ID_CMD) ? 0 : mxc_frame_counter + 1;
		buf_pop(&mxc_odata, &mxc_msg.data, buf_size(&mxc_odata));

		if(mxc_swdebug) {
			printf("\n%s: ", (can_id == CAN_ID_CMD) ? "CAN_ID_CMD":"CAN_ID_DAT");
			for(int i = 0; i < mxc_msg.dlc; i += sizeof(opcode_t)) {
				opcode_t opcode;
				memcpy(&opcode.value, mxc_msg.data+i, sizeof(opcode_t));
				vm_opcode_print(opcode);
				printf(" ");
			}
			printf("\n");
			sys_mdelay(1000);
		}
		else {
			int ecode = irc_send(&mxc_msg);
			irc_error(ecode);

			if(can_id == CAN_ID_CMD) {
				ecode = mxc_latch();
				irc_error(ecode);
			}

			if(do_measure) {
				if(mxc_flag.hv) {
					/*!!!do not power-up hv until relay settling down
					1, vm_wait 5mS
					2, mos gate delay 1mS
					*/
					vm_wait(5);

					dps_hv_start();
					mxc_measure();
					dps_hv_stop();
				}
				else {
					mxc_measure();
				}
			}
		}
	}
}

void vm_execute(opcode_t opcode, opcode_t seq)
{
	int type = opcode.type;
	int left = buf_left(&mxc_odata);
	int scan, canid;

	mxc_flag.busy = 1;

	switch(type) {
	case VM_OPCODE_GRUP:
		scan = (opcode_type_saved == VM_OPCODE_SCAN) || (opcode_type_saved == VM_OPCODE_FSCN);
		canid = (VM_SCAN_OVER_GROUP && scan && vm_get_scan_cnt()) ? CAN_ID_DAT : CAN_ID_CMD;
		mxc_emit(canid, scan);
		break;

	default:
		left -= sizeof(opcode_t);
		left -= (type == VM_OPCODE_SEQU) ? sizeof(opcode_t) : 0;
		if(left < 0) {
			//send out the data
			mxc_emit(CAN_ID_DAT, 0);
		}

		//space is enough now
		opcode_type_saved = opcode.type;
		buf_push(&mxc_odata, &opcode, sizeof(opcode_t));
		if(type == VM_OPCODE_SEQU) {
			opcode_type_saved = seq.type;
			buf_push(&mxc_odata, &seq, sizeof(opcode_t));
		}
	}

	mxc_flag.busy = 0;
}

int mxc_is_busy(void)
{
	int busy = (mxc_flag.busy) ? 1 : 0;
	busy += buf_size(&mxc_odata);
	return busy;
}

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
	mxc_flag.busy = 0;
	mxc_swdebug = 0;
	mxc_frame_counter = 0;
	buf_free(&mxc_odata);
	buf_init(&mxc_odata, 8);
	opcode_type_saved = VM_OPCODE_NULL;

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

void mxc_can_handler(can_msg_t *msg)
{
	mxc_echo_t *echo = (mxc_echo_t *) msg->data;
	int node = CAN_NODE(msg->id);
	int slot = MXC_SLOT(node);

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
	//high voltage mode?
	mxc_flag.hv = 0;
	if(mode == IRC_MODE_HVR) {
		mxc_flag.hv = 1;
	}

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
		"MXC DEBUG				MXC SOFTWARE DEBUG SWITCH\n"
		"MXC DUMP\n"
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
	else if((argc > 1) && !strcmp(argv[1], "DEBUG")) {
		mxc_swdebug = !mxc_swdebug;
		printf("mxc_swdebug is %s\r\n", mxc_swdebug ? "on" : "off");
		return 0;
	}
	else if(!strcmp(argv[1], "DUMP")) {
		//dump odata
		const char *id_type = ((mxc_msg.id & 0xFF0) == CAN_ID_CMD) ? "CAN_ID_CMD":"CAN_ID_???";
		id_type = ((mxc_msg.id & 0xFF0) == CAN_ID_DAT) ? "CAN_ID_DAT": id_type;
		printf("CAN: %s(0x%03x) ", id_type, mxc_msg.id);
		for(int i = 0; i < mxc_msg.dlc; i += sizeof(opcode_t)) {
			opcode_t opcode;
			memcpy(&opcode.value, mxc_msg.data+i, sizeof(opcode_t));
			vm_opcode_print(opcode);
			printf(" ");
		}
		printf("\n");

		//dump matrix vitrual machine
		vm_dump();
		return 0;
	}
	else if((argc > 0) && !strcmp(argv[1], "HELP")) {
		printf("%s", usage);
	}

	return 0;
}
const cmd_t cmd_mxc = {"MXC", cmd_mxc_func, "mxc related debug cmds"};
DECLARE_SHELL_CMD(cmd_mxc)
