/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd.h"
#include "sm/stepmotor.h"
#include "key.h"

//private functions
static int dlg_GetSteps(void);
static int dlg_GetMode(void);
static int dlg_GetRPM(void);
static int dlg_GetAutoSteps(void);

static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_Run(const osd_command_t *cmd);
static int dlg_ResetStep(const osd_command_t *cmd);
static int dlg_ChangeRunMode(const osd_command_t *cmd);
static int dlg_ChangeRPM(const osd_command_t *cmd);
static int dlg_ChangeAutoSteps(const osd_command_t *cmd);

const char str_status[] = "status";
const char str_rpm[] = "rpm";
const char str_cfg[] = "config";
const char str_steps[] = "steps";

const osd_item_t items_status[] = {
	{5, 1, 6, 1, (int)str_status, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 3, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 8, 1, (int)dlg_GetSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{12, 1, 4, 1, (int)dlg_GetMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_rpm[] = {
	{5, 1, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 3, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{12, 1, 4, 1, (int)dlg_GetRPM, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_steps[] = {
	{5, 1, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 5, 1, (int)str_steps, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{8, 1, 8, 1, (int)dlg_GetAutoSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_status[] = {
	{.cmd = KEY_UP, .func = dlg_SelectGroup},
	{.cmd = KEY_DOWN, .func = dlg_SelectGroup},
	{.cmd = KEY_RIGHT, .func = dlg_Run},
	{.cmd = KEY_LEFT, .func = dlg_Run},
	{.cmd = KEY_RESET, .func = dlg_ResetStep},
	{.cmd = KEY_ENTER, .func = dlg_ChangeRunMode},
	NULL,
};

const osd_command_t cmds_rpm[] = {
	{.cmd = KEY_UP, .func = dlg_SelectGroup},
	{.cmd = KEY_DOWN, .func = dlg_SelectGroup},
	{.cmd = KEY_RIGHT, .func = dlg_ChangeRPM},
	{.cmd = KEY_LEFT, .func = dlg_ChangeRPM},
	{.cmd = KEY_RESET, .func = dlg_ChangeRPM},
	{.cmd = KEY_ENTER, .func = dlg_ChangeRPM},
	NULL,
};

const osd_command_t cmds_steps[] = {
	{.cmd = KEY_UP, .func = dlg_SelectGroup},
	{.cmd = KEY_DOWN, .func = dlg_SelectGroup},
	{.cmd = KEY_RIGHT, .func = dlg_ChangeAutoSteps},
	{.cmd = KEY_LEFT, .func = dlg_ChangeAutoSteps},
	{.cmd = KEY_RESET, .func = dlg_ChangeAutoSteps},
	{.cmd = KEY_ENTER, .func = dlg_ChangeAutoSteps},
	NULL,
};

const osd_group_t sm_grps[] = {
	{.items = items_status, .cmds = cmds_status, .status = 0, .option = 0},
	{.items = items_rpm, .cmds = cmds_rpm, .status = 1, .option = 0},
	{.items = items_steps, .cmds = cmds_steps, .status = 2, .option = 0},
	NULL,
};

const osd_dialog_t sm_dlg = {
	.grps = sm_grps,
	.cmds = NULL,
	.func = NULL,
	.max_grps = 1,
};

static int dlg_GetSteps(void)
{
	return 0;
}

static int dlg_GetMode(void)
{
	return 0;
}

static int dlg_GetRPM(void)
{
	return 0;
}

static int dlg_GetAutoSteps(void)
{
	return 0;
}


static int dlg_SelectGroup(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_Run(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ResetStep(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeRunMode(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeRPM(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeAutoSteps(const osd_command_t *cmd)
{
	return 0;
}
