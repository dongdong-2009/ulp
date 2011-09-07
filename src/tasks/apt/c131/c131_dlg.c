/*
 * david@2011 initial version
 */
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "c131_relay.h"
#include "key.h"
#include "c131_misc.h"

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

enum {
	DIAGNOSIS_NOTYET,
	DIAGNOSIS_OVER,
};

//private functions
static int dlg_SelectGroup(const osd_command_t *cmd);
static int dlg_SelectSDMType(const osd_command_t *cmd);
static int dlg_SelectAPTMode(const osd_command_t *cmd);
static int dlg_SelectAPTDiag(const osd_command_t *cmd);

static int GetSDMType(void);
static int dlg_GetInfo(void);

static int index_load;
static char sdmtype_ram[16];	//the last char for EOC
static char type_select_ok;

static char aptmode_ram[10];
static char mode_select_ok = 0x7f;

static char aptdiag_ram[16];	//the last char for EOC
static char status_diag;

const char str_sdmtype[] = "SDM Type Select";
const char str_aptmode[] = "APT Mode Select";
const char str_aptdiag[] = "APT Diagnosis";
const char str_aptstatus[] = "APT Status";
const char str_type[] = "Type:";
const char str_mode[] = "Mode:";
const char str_info[] = "Info:";

const osd_item_t items_sdmtype[] = {
	{2, 0, 15, 1, (int)str_sdmtype, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 2, 16, 1, (int)sdmtype_ram, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	{19, 2, 1, 1, (int)&type_select_ok, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	NULL,
};

const osd_item_t items_aptmode[] = {
	{2, 4, 15, 1, (int)str_aptmode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 6, 10, 1, (int)aptmode_ram, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	{19, 6, 1, 1, (int)&mode_select_ok, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	NULL,
};

const osd_item_t items_aptdiag[] = {
	{3, 8, 14, 1, (int)str_aptdiag, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 10, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 10, 15, 1, (int)aptdiag_ram, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	NULL,
};

const osd_item_t items_aptstatus[] = {
	{4, 12, 10, 1, (int)str_aptstatus, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 13, 5, 1, (int)str_type, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{5, 13, 15, 1, (int)sdmtype_ram, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	{0, 14, 5, 1, (int)str_mode, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{6, 14, 10, 1, (int)aptmode_ram, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_AFTERCOMMAND, ITEM_RUNTIME_NONE},
	{0, 15, 5, 1, (int)str_info, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	// {0, 13, 5, 1, (int)dlg_GetInfo, ITEM_DRAW_TXT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
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
	{.event = KEY_RIGHT, .func = NULL},
	{.event = KEY_LEFT, .func = NULL},
	{.event = KEY_RESET, .func = NULL},
	{.event = KEY_ENTER, .func = NULL},
	NULL,
};

const osd_group_t c131_grps[] = {
	{.items = items_sdmtype, .cmds = cmds_type, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptmode, .cmds = cmds_mode, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptdiag, .cmds = cmds_diag, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_aptstatus, .cmds = cmds_status, .order = 3, .option = GROUP_DRAW_FULLSCREEN},
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

static int dlg_SelectSDMType(const osd_command_t *cmd)
{
	if (cmd->event == KEY_LEFT) {
		do {
			index_load --;
			if (index_load < 0)
				index_load = NUM_OF_LOAD - 1;
		} while(GetSDMType());
	} else if(cmd->event == KEY_RIGHT) {
		do {
			index_load ++;
			if (index_load >= NUM_OF_LOAD) 
				index_load = 0;
		} while(GetSDMType());
	} else if(cmd->event == KEY_ENTER) {
		c131_ConfirmLoad(index_load);
	}
	if (c131_GetCurrentLoadIndex() == index_load)
		type_select_ok = 0xff;
	else
		type_select_ok = 0x7f;


	return 0;
}

static int GetSDMType(void)
{
	c131_load_t * pload;
	int len;

	if (c131_GetLoad(&pload, index_load) == 0) {
		if(!pload->load_bExist)
			return 1;
		else {
			memcpy(sdmtype_ram, pload->load_name, 15);
			sdmtype_ram[15] = 0x00;
			len = strlen(sdmtype_ram);
			memset(&sdmtype_ram[len], ' ', 15 - len);
		}
	}

	return 0;
}

static int dlg_SelectAPTMode(const osd_command_t *cmd)
{
	if (cmd->event == KEY_LEFT) {
		memset(aptmode_ram, 0x00, 10);
		strcpy(aptmode_ram, "Normal   ");
		if (Get_C131Mode() == C131_MODE_NORMAL)
			mode_select_ok = 0xff;
	} else if(cmd->event == KEY_RIGHT) {
		memset(aptmode_ram, 0x00, 10);
		strcpy(aptmode_ram, "Simulator");
	} else if(cmd->event == KEY_ENTER) {
		if (Get_C131Mode() == C131_MODE_SIMULATOR)
			mode_select_ok = 0xff;
		if (strcmp(aptmode_ram, "Simulator") == 0) {
			Set_C131Mode(C131_MODE_SIMULATOR);
		} else {
			Set_C131Mode(C131_MODE_NORMAL);
		}
		mode_select_ok = 0xff;
	}

	return 0;
}

static int dlg_SelectAPTDiag(const osd_command_t *cmd)
{
	if (cmd->event == KEY_ENTER) {
		c131_DiagSW();
		c131_DiagLED();
		c131_DiagLOOP();
	}

	return 0;
}

static int dlg_GetInfo(void)
{

	return NULL;
}

static void c131_Init(void)
{
	int hdlg;

	//lowlevel app init
	c131_relay_Init();
	c131_misc_Init();

	//init dialogue
	hdlg = osd_ConstructDialog(&c131_dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);

	//init varibles
	index_load = 0;
	index_load = c131_GetCurrentLoadIndex();
	type_select_ok = 0xff;

	if (Get_C131Mode() == C131_MODE_NORMAL)
		strcpy(aptmode_ram, "Normal   ");
	else
		strcpy(aptmode_ram, "Simulator");
	mode_select_ok = 0xff;

	strcpy(aptdiag_ram, "Enter -> Start");
	status_diag = DIAGNOSIS_NOTYET;

	//app init
	GetSDMType();
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
