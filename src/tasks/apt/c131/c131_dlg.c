/*
 * david@2011 initial version
 */
#include "config.h"
#include "osd/osd.h"
#include "c131_relay.h"
#include "key.h"
#include "stm32f10x.h"

#define DELAY_MS 1000
#define REPEAT_MS 10

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
const char str_autosteps[] = "autosteps";
const char str_manu[] = "manu";
const char str_auto[] = "auto";
const char str_err[] = "err";

const osd_item_t items_status[] = {
	{5, 0, 6, 1, (int)str_status, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 6, 1, (int)dlg_GetSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{12, 1, 4, 1, (int)dlg_GetRunMode, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_rpm[] = {
	{5, 2, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 3, 3, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{8, 3, 8, 1, (int)dlg_GetRPM, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_steps[] = {
	{5, 4, 6, 1, (int)str_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 5, 9, 1, (int)str_autosteps, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
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
	{.event = KEY_MENU, .func = dlg_ResetStep},
	{.event = KEY_OSD, .func = dlg_ChangeRunMode},
	NULL,
};

const osd_command_t cmds_rpm[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeRPM},
	{.event = KEY_LEFT, .func = dlg_ChangeRPM},
	{.event = KEY_RESET, .func = dlg_ChangeRPM},
	{.event = KEY_ENTER, .func = dlg_ChangeRPM},
	{.event = KEY_MENU, .func = dlg_ChangeRPM},
	{.event = KEY_OSD, .func = dlg_ChangeRPM},
	{.event = KEY_0, .func = dlg_ChangeRPM},
	{.event = KEY_1, .func = dlg_ChangeRPM},
	{.event = KEY_2, .func = dlg_ChangeRPM},
	{.event = KEY_3, .func = dlg_ChangeRPM},
	{.event = KEY_4, .func = dlg_ChangeRPM},
	{.event = KEY_5, .func = dlg_ChangeRPM},
	{.event = KEY_6, .func = dlg_ChangeRPM},
	{.event = KEY_7, .func = dlg_ChangeRPM},
	{.event = KEY_8, .func = dlg_ChangeRPM},
	{.event = KEY_9, .func = dlg_ChangeRPM},
	NULL,
};

const osd_command_t cmds_steps[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeAutoSteps},
	{.event = KEY_LEFT, .func = dlg_ChangeAutoSteps},
	{.event = KEY_RESET, .func = dlg_ChangeAutoSteps},
	{.event = KEY_ENTER, .func = dlg_ChangeAutoSteps},
	{.event = KEY_MENU, .func = dlg_ChangeAutoSteps},
	{.event = KEY_OSD, .func = dlg_ChangeAutoSteps},
	{.event = KEY_0, .func = dlg_ChangeAutoSteps},
	{.event = KEY_1, .func = dlg_ChangeAutoSteps},
	{.event = KEY_2, .func = dlg_ChangeAutoSteps},
	{.event = KEY_3, .func = dlg_ChangeAutoSteps},
	{.event = KEY_4, .func = dlg_ChangeAutoSteps},
	{.event = KEY_5, .func = dlg_ChangeAutoSteps},
	{.event = KEY_6, .func = dlg_ChangeAutoSteps},
	{.event = KEY_7, .func = dlg_ChangeAutoSteps},
	{.event = KEY_8, .func = dlg_ChangeAutoSteps},
	{.event = KEY_9, .func = dlg_ChangeAutoSteps},
	NULL,
};

const osd_group_t sm_grps[] = {
	{.items = items_status, .cmds = cmds_status, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_rpm, .cmds = cmds_rpm, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_steps, .cmds = cmds_steps, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t sm_dlg = {
	.grps = sm_grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int dlg_GetSteps(void)
{
	return sm_GetSteps();
}

static int dlg_GetRunMode(void)
{
	const char *str;
	int mode;
	
	mode = sm_GetRunMode();
	switch (mode) {
	case SM_RUNMODE_AUTO:
		str = str_auto;
		break;
	case SM_RUNMODE_MANUAL:
		str = str_manu;
		break;
	default:
		str = str_err;
	}

	return (int) str;
}

static int dlg_GetRPM(void)
{
	return sm_GetRPM();
}

static int dlg_GetAutoSteps(void)
{
	return sm_GetAutoSteps();
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

static int dlg_Run(const osd_command_t *cmd)
{
	if(sm_GetRunMode() == SM_RUNMODE_MANUAL) {
		key_SetKeyScenario(0, REPEAT_MS);
	}
	
	sm_StartMotor((cmd->event == KEY_RIGHT)? 0 : 1);
	
	return 0;
}

static int dlg_ResetStep(const osd_command_t *cmd)
{
	sm_ResetStep();
	
	return 0;
}

static int dlg_ChangeRunMode(const osd_command_t *cmd)
{
	int ret;
	int mode;
	
	mode = sm_GetRunMode();
	mode ++;
	if(mode >= SM_RUNMODE_INVALID)
		mode = 0;
	
	ret = sm_SetRunMode(mode);
	return ret;
}

static int dlg_ChangeRPM(const osd_command_t *cmd)
{
	int result = -1;
	int rpm = sm_GetRPM();
	
	switch(cmd->event){
	case KEY_LEFT:
		rpm --;
		result = sm_SetRPM(rpm);
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		rpm ++;
		result = sm_SetRPM(rpm);
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
	case KEY_MENU:
		/*get config from flash*/
		sm_GetConfigFromFlash();
		rpm = sm_GetRPM();
		result = sm_SetRPM(rpm);
		break;
	case KEY_ENTER:
	case KEY_OSD:
		/*store config to flash*/
		sm_SaveConfigToFlash();
		break;
	default:
		rpm = key_SetEntryAndGetDigit();
		result = sm_SetRPM(rpm);
		break;
	}

	return result;
}

static int dlg_ChangeAutoSteps(const osd_command_t *cmd)
{
	int result = -1;
	int steps = sm_GetAutoSteps();
	
	switch(cmd->event){
	case KEY_LEFT:
		steps --;
		result = sm_SetAutoSteps(steps);
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		steps ++;
		result = sm_SetAutoSteps(steps);
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
	case KEY_MENU:
		/*get config from flash*/
		sm_GetConfigFromFlash();
		steps = sm_GetAutoSteps();
		result = sm_SetAutoSteps(steps);
		break;
	case KEY_ENTER:
	case KEY_OSD:
		/*store config to flash*/
		sm_SaveConfigToFlash();
		break;
	default:
		steps = key_SetEntryAndGetDigit();
		result = sm_SetAutoSteps(steps);
		break;
	}
	
	return result;
}
