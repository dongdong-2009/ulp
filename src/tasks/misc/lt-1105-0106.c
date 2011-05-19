#include "config.h"
#include "debug.h"
#include "time.h"
#include "nvm.h"
#include "key.h"
#include "osd/osd.h"
#include "sys/task.h"
#include "sys/sys.h"

#define DELAY_MS 1000
#define REPEAT_MS 10

static int loops_up_limit __nvm;
static int loops_dn_limit __nvm;
static int loops_counter = 0;

const char str_up_limit[] = "1";
const char str_dn_limit[] = "4";
const char str_loops[] = "0";

//display
static int get_up_limit(void);
static int get_dn_limit(void);
static int get_loops(void);

//modify settings
static int sel_group(const osd_command_t *cmd);
static int set_up_limit(const osd_command_t *cmd);
static int set_dn_limit(const osd_command_t *cmd);

const osd_item_t items_up_limit[] = {
	{0, 2, 1, 1, (int)str_up_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 2, 8, 1, (int)get_up_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_dn_limit[] = {
	{0, 1, 1, 1, (int)str_dn_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 8, 1, (int)get_dn_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_loops[] = {
	{0, 0, 1, 1, (int)str_loops, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 0, 8, 1, (int)get_loops, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_up_limit[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	{.event = KEY_RIGHT, .func = set_up_limit},
	{.event = KEY_LEFT, .func = set_up_limit},
	{.event = KEY_RESET, .func = set_up_limit},
	{.event = KEY_ENTER, .func = set_up_limit},
	{.event = KEY_0, .func = set_up_limit},
	{.event = KEY_1, .func = set_up_limit},
	{.event = KEY_2, .func = set_up_limit},
	{.event = KEY_3, .func = set_up_limit},
	{.event = KEY_4, .func = set_up_limit},
	{.event = KEY_5, .func = set_up_limit},
	{.event = KEY_6, .func = set_up_limit},
	{.event = KEY_7, .func = set_up_limit},
	{.event = KEY_8, .func = set_up_limit},
	{.event = KEY_9, .func = set_up_limit},
	NULL,
};

const osd_command_t cmds_dn_limit[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	{.event = KEY_RIGHT, .func = set_dn_limit},
	{.event = KEY_LEFT, .func = set_dn_limit},
	{.event = KEY_RESET, .func = set_dn_limit},
	{.event = KEY_ENTER, .func = set_dn_limit},
	{.event = KEY_0, .func = set_dn_limit},
	{.event = KEY_1, .func = set_dn_limit},
	{.event = KEY_2, .func = set_dn_limit},
	{.event = KEY_3, .func = set_dn_limit},
	{.event = KEY_4, .func = set_dn_limit},
	{.event = KEY_5, .func = set_dn_limit},
	{.event = KEY_6, .func = set_dn_limit},
	{.event = KEY_7, .func = set_dn_limit},
	{.event = KEY_8, .func = set_dn_limit},
	{.event = KEY_9, .func = set_dn_limit},
	NULL,
};

const osd_command_t cmds_loops[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	NULL,
};

const osd_group_t grps[] = {
	{.items = items_up_limit, .cmds = cmds_up_limit, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_dn_limit, .cmds = cmds_dn_limit, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_loops, .cmds = cmds_loops, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t dlg = {
	.grps = grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int sel_group(const osd_command_t *cmd)
{
	int result = -1;

	if(cmd->event == KEY_UP)
		result = osd_SelectPrevGroup();
	else
		result = osd_SelectNextGroup();

	return result;
}

static int get_up_limit(void)
{
	return loops_up_limit;
}

static int get_dn_limit(void)
{
	return loops_dn_limit;
}

static int get_loops(void)
{
	return loops_counter;
}

static int set_up_limit(const osd_command_t *cmd)
{
	int loops = loops_up_limit;

	switch(cmd -> event){
	case KEY_LEFT:
		loops = (loops > 0) ? loops - 1 : 0;
		loops_up_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		loops ++;
		loops_up_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
		break;
	case KEY_ENTER:
		nvm_save();
		break;
	default:
		loops = key_SetEntryAndGetDigit();
		loops_up_limit = loops;
		break;
	}

	return 0;
}

static int set_dn_limit(const osd_command_t *cmd)
{
	int loops = loops_dn_limit;

	switch(cmd -> event){
	case KEY_LEFT:
		loops = (loops > 0) ? loops - 1 : 0;
		loops_dn_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		loops ++;
		loops_dn_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
		break;
	case KEY_ENTER:
		nvm_save();
		break;
	default:
		loops = key_SetEntryAndGetDigit();
		loops_dn_limit = loops;
		break;
	}

	return 0;
}

void vhd_Init(void)
{
	int hdlg = osd_ConstructDialog(&dlg);
	osd_SetActiveDialog(hdlg);
}

void vhd_Update(void)
{

}

void main(void)
{
	task_Init();
	vhd_Init();
	while(1) {
		task_Update();
		vhd_Update();
	}
}