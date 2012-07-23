/*
 * 	miaofng@2009 initial version
 */

#include <string.h>
#include "config.h"
#include "osd/osd.h"
#include "stepmotor.h"
#include "key.h"
#include "stm32f10x.h"
#include "nvm.h"

#define DELAY_MS 1000
#define REPEAT_MS 50  // this para is restricted by fresh frq of lcd

//private functions
static int dlg_GetSteps(void);
static int dlg_GetRunMode(void);
static int dlg_GetRPM(void);
static int dlg_GetAutoSteps(void);
static int dlg_GetStepMode(void);
static int dlg_GetDecayMode(void);

static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_Run(const osd_command_t *cmd);
static int dlg_ResetStep(const osd_command_t *cmd);
static int dlg_ChangeRunMode(const osd_command_t *cmd);
static int dlg_ChangeRPM(const osd_command_t *cmd);
static int dlg_ChangeAutoSteps(const osd_command_t *cmd);
static int dlg_ChangeStepMode(const osd_command_t *cmd);
static int dlg_ChangeDecayMode(const osd_command_t *cmd);

static const char str_status[] = "status";
static const char str_rpm[] = "rpm";
static const char str_rpm_cfg[] = "rpm config";
static const char str_auto_cfg[] = "auto config";
static const char str_stepmode_cfg[] = "step mode";
static const char str_decaymode_cfg[] = "decay mode";
static const char str_autosteps[] = "step";
static const char str_manu[] = "manu";
static const char str_auto[] = "auto";
static const char str_err[] = "err";
static const char str_full[] = "full";
static const char str_half[] = "half";
static const char str_fast[] = "fast";
static const char str_slow[] = "slow";

const osd_item_t items_status[] = {
	{5, 0, 6, 1, (int)str_status, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 4, 1, (int)dlg_GetRunMode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{10, 1, 6, 1, (int)dlg_GetSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_rpm[] = {
	{3, 2, 10, 1, (int)str_rpm_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 3, 4, 1, (int)str_rpm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 3, 6, 1, (int)dlg_GetRPM, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_steps[] = {
	{2, 4, 11, 1, (int)str_auto_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 5, 4, 1, (int)str_autosteps, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{10, 5, 6, 1, (int)dlg_GetAutoSteps, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_stepmode[] = {
	{4, 6, 9, 1, (int)str_stepmode_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 7, 4, 1, (int)dlg_GetStepMode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_decaymode[] = {
	{4, 8, 10, 1, (int)str_decaymode_cfg, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 9, 4, 1, (int)dlg_GetDecayMode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
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

const osd_command_t cmds_stepmode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeStepMode},
	{.event = KEY_LEFT, .func = dlg_ChangeStepMode},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_command_t cmds_decaymode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ChangeDecayMode},
	{.event = KEY_LEFT, .func = dlg_ChangeDecayMode},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_group_t sm_grps[] = {
	{.items = items_status, .cmds = cmds_status, .order = 0},
	{.items = items_rpm, .cmds = cmds_rpm, .order = 1},
	{.items = items_steps, .cmds = cmds_steps, .order = 2},
	{.items = items_stepmode, .cmds = cmds_stepmode, .order = 3},
	{.items = items_decaymode, .cmds = cmds_decaymode, .order = 4},
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

static int dlg_GetRPM(void)
{
	return sm_GetRPM();
}

static int dlg_GetAutoSteps(void)
{
	return sm_GetAutoSteps();
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
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
	}
	sm_StartMotor((cmd->event == KEY_RIGHT)? 1 : 0);

	return 0;
}

static int dlg_ResetStep(const osd_command_t *cmd)
{
	sm_ResetStep();
	return 0;
}

static int dlg_ChangeRunMode(const osd_command_t *cmd)
{
	int ret, mode;

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
		nvm_init();
		rpm = sm_GetRPM();
		result = sm_SetRPM(rpm);
		break;
	case KEY_ENTER:
	case KEY_OSD:
		/*store config to flash*/
		nvm_save();
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
		nvm_init();
		steps = sm_GetAutoSteps();
		result = sm_SetAutoSteps(steps);
		break;
	case KEY_ENTER:
	case KEY_OSD:
		/*store config to flash*/
		nvm_save();
		break;
	default:
		steps = key_SetEntryAndGetDigit();
		result = sm_SetAutoSteps(steps);
		break;
	}
	
	return result;
}

static int dlg_ChangeStepMode(const osd_command_t *cmd)
{
	switch(cmd->event){
	case KEY_LEFT:
		sm_SetStepMode(SM_STEPMODE_FULL);
		break;
	case KEY_RIGHT:
		sm_SetStepMode(SM_STEPMODE_HALF);
		break;
	default:
		break;
	}

	return 0;
}

static int dlg_ChangeDecayMode(const osd_command_t *cmd)
{
	switch(cmd->event){
	case KEY_LEFT:
		sm_SetDecayMode(SM_DECAYMODE_FAST);
		break;
	case KEY_RIGHT:
		sm_SetDecayMode(SM_DECAYMODE_SLOW);
		break;
	default:
		break;
	}

	return 0;
}

static int dlg_GetStepMode(void)
{
	const char *str;

	if (sm_GetStepMode() == SM_STEPMODE_HALF)
		str = str_half;
	else
		str = str_full;

	return (int)str;
}

static int dlg_GetDecayMode(void)
{
	const char *str;

	if (sm_GetDecayMode() == SM_DECAYMODE_SLOW)
		str = str_slow;
	else
		str = str_fast;

	return (int)str;
}
