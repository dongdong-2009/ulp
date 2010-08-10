#include "osd/osd.h"
#include "key.h"
#include "ff.h"
#include "FreeRTOS.h"
#include <string.h>

static osd_dialog_t *pdlg_main;

static int dlg_SelectGroup(const osd_command_t *cmd);
static osd_dialog_t *dlg_CreateDialog(const osd_command_t *pcmd);
static osd_dialog_t *dlg_CreateMainDialog(void);
static osd_dialog_t *dlg_CreateSubDialog(void);
static void dlg_FreeDialog(osd_dialog_t *dlg);
static int dlg_ActiveMainDialog(const osd_command_t *cmd);
static int dlg_ActiveSubDialog(const osd_command_t *cmd);

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_NONE
};

const osd_command_t cmds_main[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ActiveSubDialog},
	NULL,
};

const osd_command_t cmds_sub[] = {
	{.event = KEY_UP, .func = dlg_SelectGroup},
	{.event = KEY_DOWN, .func = dlg_SelectGroup},
	{.event = KEY_RIGHT, .func = dlg_ActiveSubDialog},
	{.event = KEY_LEFT, .func = dlg_ActiveMainDialog},
	NULL,
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

static osd_dialog_t *dlg_CreateDialog(const osd_command_t *pcmd)
{
	int n, ngrps;
	char *fname;
	osd_dialog_t *pdlg;
	osd_group_t *pgrp;
	osd_item_t *pitem;
	FILINFO fileinfo;
	DIR fdir;
	
	//create groups, maxium 10
	pgrp = MALLOC(sizeof(osd_group_t) * 11);
	if(!pgrp)
		return NULL;
	
	//create dialog
	pdlg = MALLOC(sizeof(osd_dialog_t));
	if(!pdlg)
		return NULL;

	pdlg->grps = pgrp;
	pdlg->cmds = NULL;
	pdlg->func = NULL;
	pdlg->option = NULL;
	
	if(f_opendir(&fdir, ".") != FR_OK)
		return NULL;
	
	//create items & groups
	ngrps = 0;
	while(f_readdir(&fdir, &fileinfo) == FR_OK) {
		if(!(fileinfo.fattrib & AM_DIR))
			continue;

		if(fileinfo.fname[0] == '.')
			continue;
			
		if(fileinfo.fname[0] == 0)
			break;
		
		pitem = MALLOC(sizeof(osd_item_t) * 2);
		if(!pitem)
			return NULL;
		
		//malloc a space to store the folder name
		n = strlen(fileinfo.fname);
		fname = MALLOC(n + 1);
		if(!fname)
			return NULL;
		strcpy(fname, fileinfo.fname);
		
		pitem->x = 2;
		pitem->y = ngrps;
		pitem->w = 15;
		pitem->h = 1;
		pitem->v = (int)fname;
		pitem->draw = ITEM_DRAW_TXT;
		pitem->option = ITEM_ALIGN_LEFT;
		pitem->update = ITEM_UPDATE_NEVER;
		pitem->runtime = ITEM_RUNTIME_NONE;
		
		//fill NULL to last item
		memset(pitem + 1, NULL, sizeof(osd_item_t));
		
		//insert item into the group
		pgrp->items = pitem;
		pgrp->cmds = pcmd;
		pgrp->order = ngrps;
		pgrp->option = 0;
		
		ngrps ++;
		pgrp ++;
	}
	
	//fill NULL to last group
	memset(pgrp, NULL, sizeof(osd_group_t));
	return pdlg;
}

//create main dialog in heap
static osd_dialog_t *dlg_CreateMainDialog(void)
{
	f_chdir("/");
	return dlg_CreateDialog(cmds_main);
}

//create sub dialog
static osd_dialog_t *dlg_CreateSubDialog(void)
{
	return NULL;
}

static void dlg_FreeDialog(osd_dialog_t *dlg)
{
}

static int dlg_ActiveMainDialog(const osd_command_t *cmd)
{
	int result = -1;
	int hdlg;
	
	if(!pdlg_main) {
		pdlg_main = dlg_CreateMainDialog();
	}

	if(pdlg_main) {
		hdlg = osd_ConstructDialog(pdlg_main);
		osd_SetActiveDialog(hdlg);
		result = 0;
	}

	return result;
}

static int dlg_ActiveSubDialog(const osd_command_t *cmd)
{
	int result = -1;
	return result;
}

void dlg_init(void)
{
	//set key map
	key_SetLocalKeymap(keymap);
	
	pdlg_main = 0;
	dlg_ActiveMainDialog(0);
}
