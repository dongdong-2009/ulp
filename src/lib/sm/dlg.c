/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd.h"
#include "sm/stepmotor.h"
#include "key.h"
#include "stm32f10x.h"
#include "capture.h"

//private varibles for run mode
static int sm_runmode = 0; //0 = manual, 1 = auto
const char runmode_manual[] = "manual";
const char runmode_auto[] = "auto";

//private varibles for auto steps
static unsigned short sm_autosteps = 1000;

//private varibles for rpm config

//private functions
static int dlg_GetSteps(void);
static int dlg_GetRunMode(void);
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
	{5, 0, 6, 1, (int)str_status, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 8, 1, (int)dlg_GetSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{12, 1, 4, 1, (int)dlg_GetRunMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_rpm[] = {
	{5, 2, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 3, 3, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{12, 3, 4, 1, (int)dlg_GetRPM, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_steps[] = {
	{5, 4, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 5, 5, 1, (int)str_steps, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{8, 5, 8, 1, (int)dlg_GetAutoSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_status[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_Run},
	{.event = KEY_LEFT, .func = dlg_Run},
	{.event = KEY_RESET, .func = dlg_ResetStep},
	{.event = KEY_ENTER, .func = dlg_ChangeRunMode},
	NULL,
};

const osd_command_t cmds_rpm[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeRPM},
	{.event = KEY_LEFT, .func = dlg_ChangeRPM},
	{.event = KEY_RESET, .func = dlg_ChangeRPM},
	{.event = KEY_ENTER, .func = dlg_ChangeRPM},
	NULL,
};

const osd_command_t cmds_steps[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeAutoSteps},
	{.event = KEY_LEFT, .func = dlg_ChangeAutoSteps},
	{.event = KEY_RESET, .func = dlg_ChangeAutoSteps},
	{.event = KEY_ENTER, .func = dlg_ChangeAutoSteps},
	NULL,
};

const osd_group_t sm_grps[] = {
	{.items = items_status, .cmds = cmds_status, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_rpm, .cmds = cmds_rpm, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_steps, .cmds = cmds_steps, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

const osd_dialog_t sm_dlg = {
	.grps = sm_grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int dlg_GetSteps(void)
{
	return capture_GetCounter();
}

static int dlg_GetRunMode(void)
{
	if(sm_runmode)
		return runmode_auto;
	else
		return runmode_manual;
}

static int dlg_GetRPM(void)
{
	return 0;
}

static int dlg_GetAutoSteps(void)
{
	return capture_GetAutoReload();
}

static int dlg_SelectGroup(const osd_command_t *cmd)
{
	if(cmd->event == KEY_UP){
		if(sm_dlg->grps == &sm_grps[2])
			sm_dlg->grps = sm_grps;
		else
			sm_dlg->grps++;
	}
	if(cmd->event == KEY_DOWN){
		if(sm_dlg->grps == sm_grps)
			sm_dlg->grps = &sm_grps[2];
		else
			sm_dlg->grps--;
	}
	
	return 0;
}

static int dlg_Run(const osd_command_t *cmd)
{
	if(cmd->event == KEY_LEFT)
		ad9833_Disable(const ad9833_t * chip);
	if(cmd->event == KEY_RIGHT)
		ad9833_Enable(const ad9833_t * chip);
	
	return 0;
}

static int dlg_ResetStep(const osd_command_t *cmd)
{
	if(cmd->event == KEY_RESET)
		capture_ResetCounter();
	
	return 0;
}

static int dlg_ChangeRunMode(const osd_command_t *cmd)
{
	if(cmd->event == KEY_ENTER)
		sm_runmode = !sm_runmode;
	
	return 0;
}

static int dlg_ChangeRPM(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_ChangeAutoSteps(const osd_command_t *cmd)
{
	switch(cmd->event){
	case KEY_LEFT:
		if(sm_autosteps>0)
			sm_autosteps--;
		else
			sm_autosteps = 0;
		break;
	case KEY_RIGHT:
		if(sm_autosteps<65535)
			sm_autosteps++;
		else
			sm_autosteps = 65535;
		break;
	case  KEY_RESET:
		sm_autosteps = 1000;
		break;
	case KEY_ENTER:
		capture_SetAutoRelaod(sm_autosteps);
		capture_ResetCounter(); //clear counter and preload now
		break;
	default:
		break;
	}
	
	return 0;
}
