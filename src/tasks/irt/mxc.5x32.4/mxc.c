/*
* design hints:
*  1, led policy: power-up/offline=>yellow flash, ready=>green flash, error=>ecode
*  2, error policy: hold le = low, then all relay operation will fail(offline isn't an error)
*  3, power-up(/plug-in): le = high, do not affect test system and no response to can frame except CAN_ID_CFG
*
*  miaofng@2014-5-10   initial version
*  miaofng@2014-6-7    slot board v2.2, line switch default connect to external
*  miaofng@2014-9-16   slot board v3.0
*  1, alone opcode is not allowed
*  2, add FSCN sequ type support (line:bus -> line:bus)
*  3, add vbus0..3, vline support
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "common/bitops.h"
#include "mxc.h"

static int irc_mode;
static int mxc_addr;
static int mxc_line_min;
static int mxc_line_max;
static int mxc_fscn_min; //line:bus
static int mxc_fscn_max;

static int mxc_ecode;
static int mxc_le_timeout;
static time_t mxc_timer;
static int mxc_flag_ping;
static int mxc_safelatch;

static enum mxc_status_e {
	MXC_STATUS_INIT,
	MXC_STATUS_OFFLINE,
	MXC_STATUS_READY,
	MXC_STATUS_ERROR,
	MXC_STATUS_LOST,
} mxc_status = MXC_STATUS_INIT;

static int mxc_status_change(enum mxc_status_e new_status)
{
	mxc_status = new_status;

	switch(new_status) {
	case MXC_STATUS_INIT:
		le_lock();
		led_flash(LED_YELLOW);
		break;
	case MXC_STATUS_OFFLINE:
		le_unlock();
		led_on(LED_YELLOW);
		break;
	case MXC_STATUS_READY:
		mxc_timer = time_get(IRC_POL_MS * 2);
		led_on(LED_GREEN);
		break;
	case MXC_STATUS_LOST:
		led_flash(LED_GREEN);
		break;
	case MXC_STATUS_ERROR:
		le_lock();
		led_error(mxc_ecode);
		break;
	default:
		break;
	}
	return 0;
}

void mxc_error(int ecode) {
	//only log the first error
	if(mxc_ecode == IRT_E_OK) {
		mxc_ecode = ecode;
		mxc_status_change(MXC_STATUS_ERROR);
	}
}

int mxc_execute(void)
{
	time_t deadline = time_get(mxc_le_timeout);
	time_t suspend = time_get(IRC_UPD_MS);

	mxc_image_write();

	le_set(1);
	while(!le_is_pulsed()) {
		if(time_left(suspend) < 0) {
			sys_update();
		}
		if(mxc_le_timeout != 0) {
			if(time_left(deadline) < 0) { //host error???
				mxc_error(-IRT_E_LATCH_H);
				break;
			}
		}
	}

	//wait for irc clear le
	while(le_get() == 1) {
		if(time_left(suspend) < 0) {
			sys_update();
		}
		if(mxc_le_timeout != 0) {
			if(time_left(deadline) < 0) { //host error???
				mxc_error(-IRT_E_LATCH_L);
				break;
			}
		}
	}

	//operation finish
	le_set(0);
	return 0;
}

static void mxc_mode(mxc_cfg_t *cfg)
{
	int allow = mxc_status == MXC_STATUS_READY;
	allow |= mxc_status == MXC_STATUS_LOST;
	allow |= mxc_status == MXC_STATUS_OFFLINE;
	if(!allow) { //change mode is not allowed
		return;
	}

	irc_mode = cfg->mode;
	mxc_le_timeout = cfg->ms;
	mxc_safelatch = cfg->safelatch;
	mxc_relay_clr_all();

	switch(irc_mode) {
	case IRC_MODE_RMX:
	case IRC_MODE_RX2:
		mxc_linesw_set(-1);
		mxc_vsense_set(0);
		break;
	case IRC_MODE_DBG:
		mxc_linesw_set(cfg->line_sw);
		mxc_vsense_set(cfg->vbus_sw);
		break;
	default:
		mxc_linesw_set(0);
		mxc_vsense_set(0);
		break;
	}

	oe_set(0);
	if(!mxc_execute()) { //success
		mxc_image_store();
		mxc_status_change(MXC_STATUS_READY);
	}
}

static void mxc_ping(int slot)
{
	switch (mxc_status) {
	case MXC_STATUS_LOST:
		mxc_status_change(MXC_STATUS_READY);
	case MXC_STATUS_READY:
		mxc_timer = time_get(IRC_POL_MS * 2);
		break;
	default:
		break;
	}

	//response if addr is mine, whatever the status now
	mxc_flag_ping = (slot == mxc_addr) ? 1 : 0;
}

static void mxc_offline(can_msg_t *msg)
{
	//clean ecode
	if(mxc_status == MXC_STATUS_INIT) {
		irc_mode = IRC_MODE_OFF;
		mxc_ecode = 0;
		mxc_status_change(MXC_STATUS_OFFLINE);
	}
}

static void mxc_can_cfg(can_msg_t *msg)
{
	mxc_cfg_t *cfg = (mxc_cfg_t *) msg->data;
	int node = CAN_NODE(msg->id);
	int slot = MXC_SLOT(node);
	slot = (slot == MXC_SLOT_ALL) ? mxc_addr : slot;
	if(slot != mxc_addr) {
		return;
	}

	switch(cfg->cmd) {
	case MXC_CMD_RESET:
		board_reset();
		break;
	case MXC_CMD_MODE:
		mxc_mode(cfg);
		break;
	case MXC_CMD_OFFLINE:
		mxc_offline(msg);
		break;
	case MXC_CMD_PING:
		mxc_ping(slot);
		break;
	default:
		break;
	}
}

static int mxc_switch(int line, int bus, int opcode)
{
	//vbus0..3&vline
	if((opcode == VM_OPCODE_SCAN) || (opcode == VM_OPCODE_FSCN)) {
		switch(irc_mode) {
		case IRC_MODE_L4R:
		case IRC_MODE_W4R:
		case IRC_MODE_RPB:
			mxc_vsense_set(1 << bus);
			break;
		case IRC_MODE_RMX:
		case IRC_MODE_RX2:
			mxc_vsense_set((1 << bus) | (1 << 7)); //also enable vline
			break;
		case IRC_MODE_DBG:
			break;
		default:
			mxc_vsense_set(0);
			break;
		}
	}

	switch(opcode) {
	case VM_OPCODE_CLOS:
	case VM_OPCODE_SCAN:
	case VM_OPCODE_FSCN:
		mxc_relay_set(line, bus, 1);
		break;
	case VM_OPCODE_OPEN:
		mxc_relay_set(line, bus, 0);
		break;
	default:
		mxc_error(-IRT_E_OPCODE);
	}
	return 0;
}

/* limitations:
1, seq operation must be inside one can frame ----- irc vm algo could ensure!
2, command frame must contain at least one relay operation ----- irc vm algo could ensure!
*/
static void mxc_can_switch(can_msg_t *msg)
{
	opcode_t *target, *opcode;
	int n = msg->dlc / sizeof(opcode_t);
	for(int i = 0; i < n; ) {
		target = opcode = (opcode_t *) msg->data + i;
		i ++;

		if(opcode->type == VM_OPCODE_SEQU) {
			if(i < n) {
				target = (opcode_t *) msg->data + i;
				i ++;
			}
			else {
				//error: alone opcode is detected!
				break;
			}
		}

		if(opcode->type == VM_OPCODE_FSCN) { //fscn
			int min = opcode->fscn.fscn;
			int max = target->fscn.fscn;
			min = (min < mxc_fscn_min) ? mxc_fscn_min : min;
			max = (max > mxc_fscn_max) ? mxc_fscn_max : max;
			for(int fscn = min - mxc_fscn_min; fscn <= max - mxc_fscn_min; fscn ++) {
				opcode->fscn.fscn = fscn;
				mxc_switch(opcode->line, opcode->bus, target->type);
			}
		} else { //normal
			int min = opcode->line;
			int max = target->line;
			min = (min < mxc_line_min) ? mxc_line_min : min;
			max = (max > mxc_line_max) ? mxc_line_max : max;
			for(int line = min - mxc_line_min; line <= max - mxc_line_min; line ++) {
				mxc_switch(line, opcode->bus, target->type);
			}
		}
	}

	if(CAN_TYPE(msg->id) == CAN_ID_CMD) {
		//dynamic group??? note: opcode types are the same inside one group
		int scan = (target->type == VM_OPCODE_SCAN) ? 1 : 0;
		scan = (target->type == VM_OPCODE_FSCN) ? 1 : scan;

		if(scan) {
			if(mxc_safelatch) {
				//back to original status, to avoid cross conduction issue
				mxc_image_select_static();
				mxc_execute();
			}

			//switch to new status
			mxc_execute();
			mxc_image_restore();
		} else {
			mxc_execute();
			mxc_image_store();
		}
	}
}

int mxc_verify_sequence(int cnt)
{
	return 0;
}

void mxc_can_handler(can_msg_t *msg)
{
	int id = CAN_TYPE(msg->id);
	int sn = CAN_NODE(msg->id);

	switch(id) {
	case CAN_ID_DAT:
	case CAN_ID_CMD:
		if(mxc_status == MXC_STATUS_READY || mxc_status == MXC_STATUS_LOST) {
			if(!mxc_verify_sequence(sn)) {
				mxc_can_switch(msg);
			}
		}
		break;
	case CAN_ID_MXC:
		mxc_can_cfg(msg);
		break;
	default:
		break;
	}
}

void mxc_init(void)
{
	board_init();
	mxc_status_change(MXC_STATUS_INIT);

	oe_set(1);
	le_lock(); //nobody could desert me :)

	irc_mode = IRC_MODE_OFF;
	mxc_addr = mxc_addr_get();
	mxc_line_min = mxc_addr * 32;
	mxc_line_max = mxc_line_min + 32 - 1;
	mxc_fscn_min = mxc_line_min << 2;
	mxc_fscn_max = mxc_fscn_min + 32*4 - 1;

	mxc_ecode = 0;
	mxc_le_timeout = IRC_RLY_MS;
	mxc_timer = time_get(mxc_addr);
	mxc_flag_ping = 0;
}

void mxc_update(void)
{
	int flag = 0;
	if(mxc_flag_ping) {
		mxc_flag_ping = 0;
		flag = 1;
	}

	switch(mxc_status) {
	case MXC_STATUS_INIT:
		if(time_left(mxc_timer) < 0) { //attention me
			mxc_timer = time_get(IRC_INT_MS);
			flag = 1;
		}
		break;
	case MXC_STATUS_READY:
		if(time_left(mxc_timer) < 0) { //i am lost :(
			mxc_status_change(MXC_STATUS_LOST);
		}
		break;
	case MXC_STATUS_ERROR:
		if(time_left(mxc_timer) < 0) { // i am fail
			mxc_timer = time_get(IRC_INT_MS);
			flag = 1;
		}
		break;
	default:
		break;
	}

	if(flag) {
		flag = (mxc_status == MXC_STATUS_INIT) ? (1 << MXC_INIT) : 0;
		flag |= (mxc_status == MXC_STATUS_OFFLINE) ? (1 << MXC_OFFLINE) : 0;
		flag |= mxc_has_diag() ? (1 << MXC_SELF) : 0;
		flag |= mxc_has_dcfm() ? (1 << MXC_DCFM) : 0;
		mxc_can_echo(mxc_addr, flag, mxc_ecode);
	}
}

void main()
{
	sys_init();
	mxc_init();
	printf("mxc sw v1.2, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		mxc_update();
	}
}
