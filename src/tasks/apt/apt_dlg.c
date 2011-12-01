/*
 * david@2011 initial version
 */
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "key.h"
#include "ulp_time.h"
#include "c131.h"

//private functions
static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_SelectSDMType(const osd_command_t *cmd);
static int dlg_SelectPWRType(const osd_command_t *cmd);
static int dlg_SelectAPTDiag(const osd_command_t *cmd);
static int dlg_SelectAPTStatus(const osd_command_t *cmd);
static int dlg_SelectSDMTest(const osd_command_t *cmd);
static int dlg_SelectSDMDTC(const osd_command_t *cmd);

const char str_aptdiag[] = "1. APT Self Diagnose";
const char str_sdmtype[] = "2. SDM Model Select ";
const char str_sdmpwr[]  = "3. SDM Power Select ";
const char str_aptsta[]  = "4. Config Status    ";
const char str_sdmtest[] = "5. SDM Function Test";
const char str_aptdtc[]  = "6. SDM DTC Display  ";
const char str_led[] = "LED:";
const char str_sdm[] = "SDM:";
const char str_type[] = "Model:";
const char str_pwr[] = "Power:";
const char str_link[] = "Link :";
const char str_info[] = "Info:";
const char str_dtc[] = "HB MB LB SODTC";

const osd_item_t items_aptdiag[] = {
	{0, 0, 20, 1, (int)str_aptdiag, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 2, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 2, 15, 1, (int)apt_GetDiagInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_sdmtype[] = {
	{0, 4, 20, 1, (int)str_sdmtype, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 6, 16, 1, (int)apt_GetSDMTypeName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 6, 1, 1, (int)apt_GetSDMTypeSelect, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_pwrtype[] = {
	{0, 8, 20, 1, (int)str_sdmpwr, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 10, 4, 1, (int)str_sdm, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 10, 8, 1, (int)apt_GetSDMPWRName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 10, 1, 1, (int)apt_GetSDMPWRIndicator, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{0, 11, 4, 1, (int)str_led, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 11, 8, 1, (int)apt_GetLEDPWRName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{19, 11, 1, 1, (int)apt_GetLEDPWRIndicator, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptstatus[] = {
	{0, 12, 20, 1, (int)str_aptsta, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 13, 6, 1, (int)str_type, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 13, 15, 1, (int)apt_GetTypeInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 14, 6, 1, (int)str_pwr, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 14, 10, 1, (int)apt_GetSDMPWRName, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	{0, 15, 6, 1, (int)str_link, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 15, 10, 1, (int)apt_GetLinkInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_apttest[] = {
	{0, 16, 20, 1, (int)str_sdmtest, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 18, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 18, 15, 1, (int)apt_GetTestInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_aptdtc[] = {
	{0, 20, 20, 1, (int)str_aptdtc, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 22, 14, 1, (int)str_dtc, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 23, 20, 1, (int)apt_GetDTCInfo, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_V},
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

const osd_command_t cmds_type[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_SelectSDMType},
	{.event = KEY_LEFT, .func = dlg_SelectSDMType},
	{.event = KEY_ENTER, .func = dlg_SelectSDMType},
	{.event = KEY_RESET, .func = dlg_SelectSDMType},
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
	{.event = KEY_ENTER, .func = dlg_SelectSDMTest},
	{.event = KEY_RESET, .func = dlg_SelectSDMTest},
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	NULL,
};

const osd_command_t cmds_dtc[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_ENTER, .func = dlg_SelectSDMDTC},
	{.event = KEY_RIGHT, .func = dlg_SelectSDMDTC},
	{.event = KEY_LEFT, .func = dlg_SelectSDMDTC},
	{.event = KEY_RESET, .func = NULL},
	NULL,
};

const osd_group_t apt_grps[] = {
	{.items = items_aptdiag, .cmds = cmds_diag, .order = 0},
	{.items = items_sdmtype, .cmds = cmds_type, .order = 1},
	{.items = items_pwrtype, .cmds = cmds_pwr, .order = 2},
	{.items = items_aptstatus, .cmds = cmds_status, .order = 3},
	{.items = items_apttest, .cmds = cmds_test, .order = 4},
	{.items = items_aptdtc, .cmds = cmds_dtc, .order = 5},
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

static int dlg_SelectAPTDiag(const osd_command_t *cmd)
{
	return apt_SelectAPTDiag(cmd->event);
}

static int dlg_SelectAPTStatus(const osd_command_t *cmd)
{
	return 0;
}

static int dlg_SelectSDMTest(const osd_command_t *cmd)
{
	return apt_SelectSDMTest(cmd->event);
}

static int dlg_SelectSDMDTC(const osd_command_t *cmd)
{
	return apt_SelectSDMDTC(cmd->event);
}
