/*
 * david@2011 initial version
 */
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "key.h"
#include "time.h"
#include "c131.h"

//private functions
static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_SelectSDMType(const osd_command_t *cmd);
static int dlg_SelectPWRType(const osd_command_t *cmd);
static int dlg_SelectAPTMode(const osd_command_t *cmd);
static int dlg_SelectAPTDiag(const osd_command_t *cmd);
static int dlg_SelectAPTStatus(const osd_command_t *cmd);
static int dlg_SelectAPTTest(const osd_command_t *cmd);
static int dlg_SelectAPTDTC(const osd_command_t *cmd);

const char str_sdmtype[] = "SDM Type Select";
const char str_sdmpwr[] = "SDM Power Select";
const char str_aptmode[] = "APT Mode Select";
const char str_aptdiag[] = "APT Diagnosis";
const char str_aptstatus[] = "APT Status";
const char str_apttest[] = "APT Func Testing";
const char str_aptdtc[] = "APT DTC";
const char str_led[] = "LED:";
const char str_sdm[] = "SDM:";
const char str_type[] = "Type:";
const char str_mode[] = "Mode:";
const char str_info[] = "Info:";
const char str_link[] = "Link:";
const char str_dtc[] = "HB MB LB SODTC";

const osd_item_t items_sdmtype[] = {
	{2, 0, 15, 1, (int)str_sdmtype, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 2, 16, 1, (int)apt_GetSDMTypeName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 2, 1, 1, (int)apt_GetSDMTypeSelect, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_pwrtype[] = {
	{2, 4, 16, 1, (int)str_sdmpwr, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 6, 4, 1, (int)str_sdm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 6, 8, 1, (int)apt_GetSDMPWRName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 6, 1, 1, (int)apt_GetSDMPWRIndicator, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{0, 7, 4, 1, (int)str_led, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 7, 8, 1, (int)apt_GetLEDPWRName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 7, 1, 1, (int)apt_GetLEDPWRIndicator, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptmode[] = {
	{2, 8, 15, 1, (int)str_aptmode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 10, 10, 1, (int)apt_GetAPTModeName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 10, 1, 1, (int)&apt_GetAPTModeIndicator, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptdiag[] = {
	{3, 12, 14, 1, (int)str_aptdiag, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 14, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 14, 15, 1, (int)apt_GetDiagInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptstatus[] = {
	{4, 16, 10, 1, (int)str_aptstatus, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 17, 5, 1, (int)str_type, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 17, 15, 1, (int)apt_GetSDMTypeName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 18, 5, 1, (int)str_mode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 18, 10, 1, (int)apt_GetAPTModeName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 19, 5, 1, (int)str_link, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 19, 15, 1, (int)apt_GetLinkInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_apttest[] = {
	{2, 20, 16, 1, (int)str_apttest, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 22, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 22, 15, 1, (int)apt_GetTestInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptdtc[] = {
	{7, 24, 7, 1, (int)str_aptdtc, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 26, 14, 1, (int)str_dtc, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 27, 20, 1, (int)apt_GetDTCInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_type[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_SelectSDMType},
	{.event = KEY_LEFT, .func = dlg_SelectSDMType},
	{.event = KEY_ENTER, .func = dlg_SelectSDMType},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_command_t cmds_pwr[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_SelectPWRType},
	{.event = KEY_LEFT, .func = dlg_SelectPWRType},
	{.event = KEY_ENTER, .func = dlg_SelectPWRType},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_command_t cmds_mode[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_SelectAPTMode},
	{.event = KEY_LEFT, .func = dlg_SelectAPTMode},
	{.event = KEY_ENTER, .func = dlg_SelectAPTMode},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_command_t cmds_diag[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_ENTER, .func = dlg_SelectAPTDiag},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_command_t cmds_status[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_SelectAPTStatus},
	{.event = KEY_LEFT, .func = dlg_SelectAPTStatus},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_command_t cmds_test[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_ENTER, .func = dlg_SelectAPTTest},
	{.event = KEY_RESET, .func = dlg_SelectAPTTest},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	NULL,
};

const osd_command_t cmds_dtc[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_ENTER, .func = dlg_SelectAPTDTC},
	{.event = KEY_RIGHT, .func = dlg_SelectAPTDTC},
	{.event = KEY_LEFT, .func = dlg_SelectAPTDTC},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_group_t apt_grps[] = {
	{.items = items_sdmtype, .cmds = cmds_type, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_pwrtype, .cmds = cmds_pwr, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptmode, .cmds = cmds_mode, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptdiag, .cmds = cmds_diag, .order = 3, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptstatus, .cmds = cmds_status, .order = 4, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_apttest, .cmds = cmds_test, .order = 5, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptdtc, .cmds = cmds_dtc, .order = 6, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t apt_dlg = {
	.grps = apt_grps,
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

static int dlg_SelectSDMType(const osd_command_t *cmd)
{
	return apt_SelectSDMType(cmd->event);
}

static int dlg_SelectPWRType(const osd_command_t *cmd)
{
	return apt_SelectPWR(cmd->event);
}

static int dlg_SelectAPTMode(const osd_command_t *cmd)
{
	return apt_SelectAPTMode(cmd->event);
}

static int dlg_SelectAPTDiag(const osd_command_t *cmd)
{
	return apt_SelectAPTDiag(cmd->event);
}

static int dlg_SelectAPTStatus(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_SelectAPTTest(const osd_command_t *cmd)
{
	return apt_SelectAPTTest(cmd->event);
}

static int dlg_SelectAPTDTC(const osd_command_t *cmd)
{
	return apt_SelectAPTDTC(cmd->event);
}
