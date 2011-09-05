/*
 * david@2011 initial version
 */
#include "config.h"
#include "osd/osd.h"
#include "c131_relay.h"
#include "key.h"
#include "stm32f10x.h"
#include "sys/task.h"

//local key may
static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

//private functions
static int dlg_SelectGroup(const osd_command_t *cmd);

static int dlg_GetSDMType1(void);
static int dlg_GetSDMType2(void);
static int dlg_GetSDMType3(void);
static int dlg_GetAPTMode1(void);
static int dlg_GetAPTMode2(void);
static int dlg_GetType(void);
static int dlg_GetMode(void);
static int dlg_GetInfo(void);

const char str_sdmtype[] = "SDM Type Select";
const char str_aptmode[] = "APT Mode Select";
const char str_aptstatus[] = "APT Status";
const char str_type[] = "Type:";
const char str_mode[] = "Mode:";
const char str_info[] = "Info:";

const osd_item_t items_sdmtype[] = {
	{2, 0, 15, 1, (int)str_sdmtype, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 6, 1, (int)dlg_GetSDMType1, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 2, 6, 1, (int)dlg_GetSDMType2, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 3, 6, 1, (int)dlg_GetSDMType3, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptmode[] = {
	{2, 4, 15, 1, (int)str_aptmode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 5, 6, 1, (int)dlg_GetAPTMode1, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 6, 6, 1, (int)dlg_GetAPTMode2, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptstatus[] = {
	{4, 7, 10, 1, (int)str_aptstatus, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 8, 5, 1, (int)str_type, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	// {0, 9, 5, 1, (int)dlg_GetType, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{0, 9, 5, 1, (int)str_mode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	// {0, 11, 5, 1, (int)dlg_GetMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{0, 10, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	// {0, 13, 5, 1, (int)dlg_GetInfo, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_type[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_command_t cmds_mode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_command_t cmds_status[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_group_t c131_grps[] = {
	{.items = items_sdmtype, .cmds = cmds_type, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptmode, .cmds = cmds_mode, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptstatus, .cmds = cmds_status, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t c131_dlg = {
	.grps = c131_grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int dlg_SelectGroup(const osd_command_t *cmd)
{
	int result = -1;

	if(cmd->event == KEY_UP)
		result = osd_SelectPrevGroup();
	else
		result = osd_SelectNextGroup();

	return result;
}

static int dlg_GetSDMType1(void)
{

	return NULL;
}

static int dlg_GetSDMType2(void)
{

	return NULL;
}

static int dlg_GetSDMType3(void)
{

	return NULL;
}

static int dlg_GetAPTMode1(void)
{

	return NULL;
}

static int dlg_GetAPTMode2(void)
{

	return NULL;
}

static int dlg_GetType(void)
{

	return NULL;
}

static int dlg_GetMode(void)
{

	return NULL;
}

static int dlg_GetInfo(void)
{

	return NULL;
}

static void c131_Init(void)
{
	int hdlg;
	hdlg = osd_ConstructDialog(&c131_dlg);
	osd_SetActiveDialog(hdlg);

	//set key map
	key_SetLocalKeymap(keymap);
}

static void c131_Update(void)
{

}

void main(void)
{
	task_Init();
	c131_Init();

	while(1) {
		task_Update();
		c131_Update();
	}
}
