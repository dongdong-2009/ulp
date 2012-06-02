/*
 * 	miaofng@2011 initial version
 */

#include <string.h>
#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "sys/task.h"
#include "priv/mcamos.h"
#include "weifu_lcm.h"
#include "osd/osd.h"
#include "key.h"
#include "encoder.h"
#include "nvm.h"

static mcamos_srv_t lcm_server;
static lcm_dat_t lcm_dat __nvm;
static lcm_cfg_t lcm_cfg = {
	0, 5000, /*eng rpm*/
	-10, 10,   /*phase diff*/
	0, 5000, /*vss*/
	0, 100, /*tim sensor duty cycle*/
	250, 250, /*tim sensor frequence*/
	1500, 15000, /*flow meter*/
	17000, 19000, /*flow meter diag*/
	0, 1, /*op mode, 0->37x, 1->120x*/
};

static struct {
	unsigned char focus : 1; //focused?
} lcm_flag;

static int set_items_value(const osd_command_t *cmd);
static int get_eng_rpm(void) {return lcm_dat.eng_rpm;}
static int get_phase_diff(void) {return lcm_dat.phase_diff;}
static int get_vss(void) {return lcm_dat.vss;}
static int get_tim_dc(void) {return lcm_dat.tim_dc;}
static int get_tim_frq(void) {return lcm_dat.tim_frq;}
static int get_hfmsig(void) {return lcm_dat.hfmsig;}
static int get_hfmref(void) {return lcm_dat.hfmref;}
static int get_op_mode(void) {return lcm_dat.op_mode;}
static int get_eng_speed(void) {return lcm_dat.eng_speed;}
static int get_wtout(void) {return lcm_dat.wtout;}

const char str_eng_rpm[] = "Eng RPM";
const char str_phase_diff[] = "Phase Diff";
const char str_vss[] = "VSS";
const char str_tim_dc[] = "Tim DC";
const char str_tim_frq[] = "Tim FRQ";
const char str_hfmsig[] = "HFMSIG";
const char str_hfmref[] = "HFMRef";
const char str_op_mode[] = "OP Mode";
const char str_eng_speed[] = "Eng Speed";
const char str_wtout[] = "WTOUT";

const osd_item_t items_eng_rpm[] = {
	{0, 0, 11, 1, (int)str_eng_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 0, 4, 1, (int)get_eng_rpm, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_phase_diff[] = {
	{0, 1, 11, 1, (int)str_phase_diff, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 1, 4, 1, (int)get_phase_diff, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_vss[] = {
	{0, 2, 11, 1, (int)str_vss, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 2, 4, 1, (int)get_vss, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_tim_dc[] = {
	{0, 3, 11, 1, (int)str_tim_dc, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 3, 4, 1, (int)get_tim_dc, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_tim_frq[] = {
	{0, 4, 11, 1, (int)str_tim_frq, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 4, 4, 1, (int)get_tim_frq, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_hfmsig[] = {
	{0, 5, 10, 1, (int)str_hfmsig, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 5, 5, 1, (int)get_hfmsig, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_hfmref[] = {
	{0, 6, 10, 1, (int)str_hfmref, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 6, 5, 1, (int)get_hfmref, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_op_mode[] = {
	{0, 7, 10, 1, (int)str_op_mode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 7, 5, 1, (int)get_op_mode, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_eng_speed[] = {
	{0, 8, 10, 1, (int)str_eng_speed, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 8, 5, 1, (int)get_eng_speed, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_wtout[] = {
	{0, 9, 10, 1, (int)str_wtout, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 9, 5, 1, (int)get_wtout, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_items[] = {
	{.event = KEY_ENTER, .func = set_items_value},
	{.event = KEY_ENCODER_P, .func = set_items_value},
	{.event = KEY_ENCODER_N, .func = set_items_value},
	{.event = OSDE_GRP_FOCUS, .func = set_items_value},
	NULL,
};

const osd_group_t grps[] = {
	{.items = items_eng_rpm, .cmds = cmds_items, .order = 0, .option = 0},
	{.items = items_phase_diff, .cmds = cmds_items, .order = 1, .option = 0},
	{.items = items_vss, .cmds = cmds_items, .order = 2, .option = 0},
	{.items = items_tim_dc, .cmds = cmds_items, .order = 3, .option = 0},
	{.items = items_tim_frq, .cmds = cmds_items, .order = 4, .option = 0},
	{.items = items_hfmsig, .cmds = cmds_items, .order = 5, .option = 0},
	{.items = items_hfmref, .cmds = cmds_items, .order = 6, .option = 0},
	{.items = items_op_mode, .cmds = cmds_items, .order = 7, .option = 0},
	{.items = items_eng_speed, .cmds = cmds_items, .order = STATUS_GRAYED, .option = 0},
	{.items = items_wtout, .cmds = cmds_items, .order = STATUS_GRAYED, .option = 0},
	NULL,
};

osd_dialog_t dlg = {
	.grps = grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int set_group_focus(const osd_command_t *cmd)
{
	osd_group_t *group = osd_GetCurrentGroup();
	short *p = (short *) &lcm_cfg;
	switch(cmd->event) {
	case OSDE_GRP_FOCUS:
	case KEY_ENTER:
		lcm_flag.focus = 1;
		p += group->order << 1;
		encoder_SetRange(p[0], p[1]);
		p = (short *) &lcm_dat;
		encoder_SetValue(p[group->order]);
		break;

	case KEY_ENCODER_P:
		osd_SelectNextGroup();
		break;

	case KEY_ENCODER_N:
		osd_SelectPrevGroup();
		break;

	default:
		break;
	}
	return 0;
}

static int set_items_value(const osd_command_t *cmd)
{
	int value;
	osd_group_t *group = osd_GetCurrentGroup();
	short *p = (short *) &lcm_dat;

	if(lcm_flag.focus == 0)
		return set_group_focus(cmd);

	switch(cmd->event) {
	case OSDE_GRP_FOCUS:
		p =  (short *) &lcm_cfg;
		p += group->order << 1;
		encoder_SetRange(p[0], p[1]);
		p = (short *) &lcm_dat;
		encoder_SetValue(p[group->order]);
		break;

	case KEY_ENTER:
		lcm_flag.focus = 0;
		break;

	case KEY_ENCODER_P:
	case KEY_ENCODER_N:
		value = encoder_GetValue();
		p[group->order] = value;
		break;

	default:
		break;
	}
	return 0;
}

static void serv_init(void)
{
	lcm_server.can = &can1;
	lcm_server.id_cmd = MCAMOS_MSG_CMD_ID;
	lcm_server.id_dat = MCAMOS_MSG_DAT_ID;
	mcamos_srv_init(&lcm_server);
}

static void serv_update(void)
{
	char ret = -1;
	char *inbox = lcm_server.inbox;
	char *outbox = lcm_server.outbox + 2;

	mcamos_srv_update(&lcm_server);
	switch(inbox[0]) {
	case LCM_CMD_NONE:
		return;
	case LCM_CMD_CONFIG:
		memcpy(&lcm_cfg, inbox + 1, sizeof(lcm_cfg));
		break;
	case LCM_CMD_READ:
		memcpy(outbox, &lcm_dat, sizeof(lcm_dat));
		break;
	case LCM_CMD_WRITE:
		//only update eng_speed and wtout.
		memcpy(&(lcm_dat.eng_speed), inbox + 1, 4);
		break;
	case LCM_CMD_SAVE:
		ret = 0;
		nvm_save();
		break;
	default:
		ret = -1; //unsupported command
		break;
	}

	lcm_server.outbox[0] = inbox[0];
	lcm_server.outbox[1] = ret;
	lcm_server.inbox[0] = 0; //!!!clear inbox testid indicate cmd ops finished
}

void lcm_Init(void)
{
	static const int keymap[] = {KEY_ENTER, KEY_NONE};
	int hdlg;
	if(lcm_dat.crc != 0)
		memset(&lcm_dat, 0, sizeof(lcm_dat));
	lcm_dat.hfmsig = lcm_cfg.hfmsig_min;
	lcm_dat.hfmref = lcm_cfg.hfmref_min;
	lcm_dat.tim_frq = lcm_cfg.timfrq_min;
	lcm_dat.op_mode = lcm_cfg.op_mode_min;

	hdlg = osd_ConstructDialog(&dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);
	lcm_flag.focus = 0;
	serv_init();
}

void lcm_Update(void)
{
	serv_update();
}

int main(void)
{
	task_Init();
	lcm_Init();
	while(1) {
		task_Update();
		lcm_Update();
	}
}
