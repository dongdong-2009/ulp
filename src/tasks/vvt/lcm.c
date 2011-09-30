/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "debug.h"
#include "time.h"
#include "sys/task.h"
#include "priv/mcamos.h"
#include "lcm.h"
#include "osd/osd.h"
#include "key.h"
#include "encoder.h"

lcm_dat_t lcm_dat;
lcm_cfg_t lcm_cfg = {
	0, 8000, /*rpm*/
	-60, 60, /*cam adv*/
	0, 5000, /*wss*/
	0, 5000, /*vss*/
	0, 100, /*misfire strength*/
	0, 100, /*knock strength*/
};

static struct {
	unsigned char focus : 1; //focused?
} lcm_flag;

static int set_items_value(const osd_command_t *cmd);
static int get_rpm(void) {return lcm_dat.rpm;}
static int get_cam(void) {return lcm_dat.cam;}
static int get_wss(void) {return lcm_dat.wss;}
static int get_vss(void) {return lcm_dat.vss;}
static int get_mfr(void) {return lcm_dat.mfr;}
static int get_knk(void) {return lcm_dat.knk;}
static int get_dio(void) {return lcm_dat.dio;}

const char str_rpm[] = "RPM";
const char str_cam[] = "CAM ADV";
const char str_wss[] = "WSS";
const char str_vss[] = "VSS";
const char str_mfr[] = "MISFIRE";
const char str_knk[] = "KNOCK";
const char str_dio[] = "SW STATUS";

const osd_item_t items_rpm[] = {
	{0, 0, 11, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 0, 4, 1, (int)get_rpm, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_cam[] = {
	{0, 1, 11, 1, (int)str_cam, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 1, 4, 1, (int)get_cam, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_wss[] = {
	{0, 2, 11, 1, (int)str_wss, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 2, 4, 1, (int)get_wss, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_vss[] = {
	{0, 3, 11, 1, (int)str_vss, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 3, 4, 1, (int)get_vss, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_mfr[] = {
	{0, 4, 11, 1, (int)str_mfr, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 4, 4, 1, (int)get_mfr, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_knk[] = {
	{0, 5, 11, 1, (int)str_knk, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 5, 4, 1, (int)get_knk, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_dio[] = {
	{0, 6, 11, 1, (int)str_dio, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{11, 6, 4, 1, (int)get_dio, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_items[] = {
	{.event = KEY_ENTER, .func = set_items_value},
	{.event = KEY_ENCODER_P, .func = set_items_value},
	{.event = KEY_ENCODER_N, .func = set_items_value},
	NULL,
};

const osd_group_t grps[] = {
	{.items = items_rpm, .cmds = cmds_items, .order = 0, .option = 0},
	{.items = items_cam, .cmds = cmds_items, .order = 1, .option = 0},
	{.items = items_wss, .cmds = cmds_items, .order = 2, .option = 0},
	{.items = items_vss, .cmds = cmds_items, .order = 3, .option = 0},
	{.items = items_mfr, .cmds = cmds_items, .order = 4, .option = 0},
	{.items = items_knk, .cmds = cmds_items, .order = 5, .option = 0},
	{.items = items_dio, .cmds = cmds_items, .order = STATUS_GRAYED, .option = 0},
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

void lcm_Init(void)
{
	static const int keymap[] = {KEY_ENTER, KEY_NONE};
	int hdlg = osd_ConstructDialog(&dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);
	lcm_flag.focus = 0;
}

void lcm_Update(void)
{
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
