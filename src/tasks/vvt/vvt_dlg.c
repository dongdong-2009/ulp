/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd.h"
#include "vvt/vvt.h"
#include "key.h"
#include "stm32f10x.h"

#define DELAY_MS 1000
#define REPEAT_MS 10

//private functions
static int dlg_GetRPM(void);
static int dlg_GetRunMode(void);
static int dlg_GetCamMode(void);
static int dlg_GetCamPhase(void);
static int dlg_GetKnockPhase(void);
static int dlg_GetKnockWindow(void);
static int dlg_GetMisStren(void);
static int dlg_GetCylinderMode(void);

static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_ChangeRunMode(const osd_command_t *cmd);
static int dlg_ChangeCamMode(const osd_command_t *cmd);
static int dlg_ChangeCamPhase(const osd_command_t *cmd);
static int dlg_ChangeKnockPhase(const osd_command_t *cmd);
static int dlg_ChangeKnockWindow(const osd_command_t *cmd);
static int dlg_ChangeMisStren(const osd_command_t *cmd);
static int dlg_ChangeCylinderMode(const osd_command_t *cmd);
static int dlg_LoadConfig(const osd_command_t *cmd);
static int dlg_SaveConfig(const osd_command_t *cmd);

const char str_status[] = "status";
const char str_cam[] = "cam config";
const char str_mode[] = "mode";
const char str_phase[] = "phase";
const char str_knock[] = "knock config";
const char str_window[] = "window";
const char str_misfire[] = "misfire config";
const char str_strength[] = "stren";
const char str_cylinder[] = "cylinder config";

const osd_item_t items_status[] = {
	{5, 0, 6, 1, (int)str_status, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 5, 1, (int)dlg_GetRPM, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{13, 1, 3, 1, (int)dlg_GetRunMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_cam_mode[] = {
	{3, 2, 10, 1, (int)str_cam, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 3, 4, 1, (int)str_mode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{14, 3, 2, 1, (int)dlg_GetCamMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_cam_phase[] = {
	{3, 4, 10, 1, (int)str_cam, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 5, 5, 1, (int)str_phase, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{13, 5, 3, 1, (int)dlg_GetCamPhase, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_knock_phase[] = {
	{2, 6, 12, 1, (int)str_knock, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 7, 5, 1, (int)str_phase, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{13, 7, 3, 1, (int)dlg_GetKnockPhase, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_knock_window[] = {
	{2, 8, 12, 1, (int)str_knock, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 9, 6, 1, (int)str_window, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{14, 9, 2, 1, (int)dlg_GetKnockWindow, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_misfire_strength[] = {
	{1, 10, 14, 1, (int)str_misfire, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 11, 5, 1, (int)str_strength, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{12, 11, 4, 1, (int)dlg_GetMisStren, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_cylinder_mode[] = {
	{0, 12, 8, 1, (int)str_cylinder, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 13, 4, 1, (int)str_mode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{14, 13, 2, 1, (int)dlg_GetCylinderMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_status[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = ((void *)0)},
	{.event = KEY_LEFT, .func = ((void *)0)},
	{.event = KEY_RESET, .func = ((void *)0)},
	{.event = KEY_ENTER, .func = dlg_ChangeRunMode},
#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
	{.event = KEY_MENU, .func = ((void *)0)},
	{.event = KEY_OSD, .func = ((void *)0)},
#endif
	NULL,
};

const osd_command_t cmds_cam_mode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeCamMode},
	{.event = KEY_LEFT, .func = dlg_ChangeCamMode},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
	NULL,
};

const osd_command_t cmds_cam_phase[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeCamPhase},
	{.event = KEY_LEFT, .func = dlg_ChangeCamPhase},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
	{.event = KEY_MENU, .func = dlg_ChangeCamPhase},
	{.event = KEY_OSD, .func = dlg_ChangeCamPhase},
	{.event = KEY_0, .func = dlg_ChangeCamPhase},
	{.event = KEY_1, .func = dlg_ChangeCamPhase},
	{.event = KEY_2, .func = dlg_ChangeCamPhase},
	{.event = KEY_3, .func = dlg_ChangeCamPhase},
	{.event = KEY_4, .func = dlg_ChangeCamPhase},
	{.event = KEY_5, .func = dlg_ChangeCamPhase},
	{.event = KEY_6, .func = dlg_ChangeCamPhase},
	{.event = KEY_7, .func = dlg_ChangeCamPhase},
	{.event = KEY_8, .func = dlg_ChangeCamPhase},
	{.event = KEY_9, .func = dlg_ChangeCamPhase},
#endif
	NULL,
};

const osd_command_t cmds_knock_phase[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeKnockPhase},
	{.event = KEY_LEFT, .func = dlg_ChangeKnockPhase},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
	{.event = KEY_MENU, .func = dlg_ChangeKnockPhase},
	{.event = KEY_OSD, .func = dlg_ChangeKnockPhase},
	{.event = KEY_0, .func = dlg_ChangeKnockPhase},
	{.event = KEY_1, .func = dlg_ChangeKnockPhase},
	{.event = KEY_2, .func = dlg_ChangeKnockPhase},
	{.event = KEY_3, .func = dlg_ChangeKnockPhase},
	{.event = KEY_4, .func = dlg_ChangeKnockPhase},
	{.event = KEY_5, .func = dlg_ChangeKnockPhase},
	{.event = KEY_6, .func = dlg_ChangeKnockPhase},
	{.event = KEY_7, .func = dlg_ChangeKnockPhase},
	{.event = KEY_8, .func = dlg_ChangeKnockPhase},
	{.event = KEY_9, .func = dlg_ChangeKnockPhase},
#endif
	NULL,
};

const osd_command_t cmds_knock_window[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeKnockWindow},
	{.event = KEY_LEFT, .func = dlg_ChangeKnockWindow},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
#if CONFIG_RCKEY_PROTOCOL_RC5 == 1
	{.event = KEY_MENU, .func = dlg_ChangeKnockWindow},
	{.event = KEY_OSD, .func = dlg_ChangeKnockWindow},
	{.event = KEY_0, .func = dlg_ChangeKnockWindow},
	{.event = KEY_1, .func = dlg_ChangeKnockWindow},
	{.event = KEY_2, .func = dlg_ChangeKnockWindow},
	{.event = KEY_3, .func = dlg_ChangeKnockWindow},
	{.event = KEY_4, .func = dlg_ChangeKnockWindow},
	{.event = KEY_5, .func = dlg_ChangeKnockWindow},
	{.event = KEY_6, .func = dlg_ChangeKnockWindow},
	{.event = KEY_7, .func = dlg_ChangeKnockWindow},
	{.event = KEY_8, .func = dlg_ChangeKnockWindow},
	{.event = KEY_9, .func = dlg_ChangeKnockWindow},
#endif
	NULL,
};

const osd_command_t cmds_misfire_strength[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeMisStren},
	{.event = KEY_LEFT, .func = dlg_ChangeMisStren},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
	NULL,
};

const osd_command_t cmds_cylinder_mode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeCylinderMode},
	{.event = KEY_LEFT, .func = dlg_ChangeCylinderMode},
	{.event = KEY_RESET, .func = dlg_LoadConfig},
	{.event = KEY_ENTER, .func = dlg_SaveConfig},
	NULL,
};

const osd_group_t vvt_grps[] = {
	{.items = items_status, .cmds = cmds_status, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_cam_mode, .cmds = cmds_cam_mode, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_cam_phase, .cmds = cmds_cam_phase, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_knock_phase, .cmds = cmds_knock_phase, .order = 3, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_knock_window, .cmds = cmds_knock_window, .order = 4, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_misfire_strength, .cmds = cmds_misfire_strength, .order = 5, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_cylinder_mode, .cmds = cmds_cylinder_mode, .order = 6, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t vvt_dlg = {
	.grps = vvt_grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

//private functions
static int dlg_GetRPM(void)
{
	return 0;
}

static int dlg_GetRunMode(void)
{
	return 0;
}

static int dlg_GetCamMode(void)
{
	return 0;
}

static int dlg_GetCamPhase(void)
{
	return 0;
}

static int dlg_GetKnockPhase(void)
{
	return vvt_GetKnockPhase();
}

static int dlg_GetKnockWindow(void)
{
	return vvt_GetKnockWindow();
}

static int dlg_GetMisStren(void)
{
	return 0;
}

static int dlg_GetCylinderMode(void)
{
	return 0;
}

static int dlg_SelectGroup(const osd_command_t *cmd)
{
	int result = -1;
	
	if(cmd->event == KEY_UP)
		result = osd_SelectPrevGroup();
	else
		result = osd_SelectNextGroup();
	
	return result;
}

static int dlg_ChangeRunMode(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeCamMode(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeCamPhase(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeKnockPhase(const osd_command_t *cmd)
{
	int temp = vvt_GetKnockPhase();

	switch (cmd->event) {
		case KEY_UP:
			vvt_SetKnockPhase(++temp);
			break;
		case KEY_DOWN:
			vvt_SetKnockPhase(++temp);
			break;
		default:
			temp = key_SetEntryAndGetDigit();
			vvt_SetKnockPhase(temp);
			break;
	}
	return 0;
}

static int dlg_ChangeKnockWindow(const osd_command_t *cmd)
{
	int temp = vvt_GetKnockWindow();

	switch (cmd->event) {
		case KEY_UP:
			vvt_SetKnockWindow(++temp);
			break;
		case KEY_DOWN:
			vvt_SetKnockWindow(++temp);
			break;
		default:
			temp = key_SetEntryAndGetDigit();
			vvt_SetKnockWindow(temp);
			break;
	}
	return 0;
}

static int dlg_ChangeMisStren(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeCylinderMode(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_LoadConfig(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_SaveConfig(const osd_command_t *cmd)
{
	return 0;
}
