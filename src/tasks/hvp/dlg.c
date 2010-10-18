#include "osd/osd.h"
#include "key.h"
#include "ff.h"
#include "FreeRTOS.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "time.h"
#include "hvp.h"

#define __DEBUG_TIME

static osd_dialog_t *pdlg_main;
static int hdlg_main;
static osd_dialog_t *pdlg_sub;
static int hdlg_sub;
static char dlg_prog_msg[16];
static int dlg_prog_time;
#ifdef __DEBUG_TIME
static time_t timer;
static char dlg_flag_stop;
#endif

static int dlg_SelectGroup(const osd_command_t *cmd);
static osd_dialog_t *dlg_CreateDialog(const osd_command_t *pcmd);
static void dlg_FreeDialog(osd_dialog_t *dlg);
static int dlg_ActiveMainDialog(const osd_command_t *cmd);
static int dlg_ActiveSubDialog(const osd_command_t *cmd);
static int dlg_StartProgram(const osd_command_t *cmd);

static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_LEFT,
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
	{.event = KEY_RIGHT, .func = dlg_StartProgram},
	{.event = KEY_LEFT, .func = dlg_ActiveMainDialog},
	NULL,
};

int dlg_set_prog_step(const char *fmt, ...)
{
	va_list ap;
	char *pstr;
	int n = 0;

	pstr = dlg_prog_msg;
	va_start(ap, fmt);
	n += vsnprintf(pstr + n, 15 - n, fmt, ap);
	va_end(ap);
	
	dlg_prog_msg[15] = 0;
	return 0;
}

void dlg_prog_finish(int status)
{
	dlg_flag_stop = 1;
}

static int dlg_get_prog_addr(void)
{
#ifdef __DEBUG_TIME
	if(time_left(timer) < 0) {
			timer = time_get(1000);
			if(!dlg_flag_stop)
				dlg_prog_time ++;
	}
#endif
	return dlg_prog_time;
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

static osd_dialog_t *dlg_CreateProgDialog(void)
{
	osd_dialog_t *pdlg;
	osd_group_t *pgrp;
	osd_item_t *pitem;
	
	//step1, create 2 groups
	pgrp = MALLOC(sizeof(osd_group_t) * 3);
	if(!pgrp)
		return NULL;
	
	//step 2, create dialog
	pdlg = MALLOC(sizeof(osd_dialog_t));
	if(!pdlg)
		return NULL;

	pdlg->grps = pgrp;
	pdlg->cmds = NULL;
	pdlg->func = NULL;
	pdlg->option = NULL;
		
	//step3, create 1st items - "programming"
	pitem = MALLOC(sizeof(osd_item_t) * 2);
	if(!pitem)
		return NULL;
	
	//insert item into the group
	pgrp->items = pitem;
	pgrp->cmds = NULL;
	pgrp->order = 0;
	pgrp->option = 0;
	pgrp ++;
	
	pitem->x = 0;
	pitem->y = 0;
	pitem->w = 15;
	pitem->h = 1;
	pitem->v = (int)("programming");
	pitem->draw = ITEM_DRAW_TXT;
	pitem->option = ITEM_ALIGN_MIDDLE;
	pitem->update = ITEM_UPDATE_NEVER;
	pitem->runtime = ITEM_RUNTIME_NONE;
	pitem ++;
		
	//fill NULL to last item
	memset(pitem, NULL, sizeof(osd_item_t));
	
	//step4, create 2nd items - "step&addr"
	pitem = MALLOC(sizeof(osd_item_t) * 3);
	if(!pitem)
		return NULL;
	
	//insert item into the group
	pgrp->items = pitem;
	pgrp->cmds = NULL;
	pgrp->order = 1;
	pgrp->option = 0;
	pgrp ++;
	
	pitem->x = 0;
	pitem->y = 5;
	pitem->w = 15;
	pitem->h = 1;
	pitem->v = (int)(dlg_prog_msg);
	pitem->draw = ITEM_DRAW_TXT;
	pitem->option = ITEM_ALIGN_MIDDLE;
	pitem->update = ITEM_UPDATE_ALWAYS;
	pitem->runtime = ITEM_RUNTIME_NONE;
	pitem ++;
	
	pitem->x = 0;
	pitem->y = 6;
	pitem->w = 15;
	pitem->h = 1;
	pitem->v = (int) dlg_get_prog_addr;
	pitem->draw = ITEM_DRAW_INT;
	pitem->option = ITEM_ALIGN_MIDDLE;
	pitem->update = ITEM_UPDATE_ALWAYS;
	pitem->runtime = ITEM_RUNTIME_V;
	pitem ++;
	
	//fill NULL to last item
	memset(pitem, NULL, sizeof(osd_item_t));
	
	//step5, fill NULL to last group
	memset(pgrp, NULL, sizeof(osd_group_t));
	return pdlg;
}

static osd_dialog_t *dlg_CreateDialog(const osd_command_t *pcmd)
{
	int n, ngrps;
	char *fname;
	osd_dialog_t *pdlg;
	osd_group_t *pgrp;
	osd_item_t *pitem;
	FILINFO fileinfo;
#ifdef CONFIG_FATFS_LFN
	fileinfo.lfsize = 0;
#endif
	DIR fdir;
	
	//create groups, maxium 10
	pgrp = MALLOC(sizeof(osd_group_t) * 21);
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
		
		pitem->x = 0;
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

static void dlg_FreeDialog(osd_dialog_t *dlg)
{
}

static int dlg_ActiveMainDialog(const osd_command_t *cmd)
{
	int result = -1;
	
	f_chdir("/");
	
	//destroy subdialog if exist
	if(pdlg_sub) {
		osd_DestroyDialog(hdlg_sub);
		hdlg_sub = 0;
		
		dlg_FreeDialog(pdlg_sub);
		pdlg_sub = 0;
	}
	
	if(!pdlg_main) {
		pdlg_main = dlg_CreateDialog(cmds_main);
	}

	if(pdlg_main) {
		hdlg_main = osd_ConstructDialog(pdlg_main);
		osd_SetActiveDialog(hdlg_main);
		result = 0;
	}

	return result;
}

static int dlg_ActiveSubDialog(const osd_command_t *cmd)
{
	char *fname;
	osd_group_t *grp = osd_GetCurrentGroup();
	
	if(grp == NULL)
		return -1;

	fname = (char *)(grp->items[0].v);
	f_chdir(fname);
	
	pdlg_sub = dlg_CreateDialog(cmds_sub);
	if(pdlg_sub) {
		osd_DestroyDialog(hdlg_main);
		hdlg_sub = osd_ConstructDialog(pdlg_sub);
		osd_SetActiveDialog(hdlg_sub);
	}
	return 0;
}

static int dlg_StartProgram(const osd_command_t *cmd)
{
	char *fname;
	osd_group_t *grp = osd_GetCurrentGroup();
	
	if(grp == NULL)
		return -1;

	fname = (char *)(grp->items[0].v);
	f_chdir(fname);
	
	//destroy subdialog if exist
	if(pdlg_sub) {
		osd_DestroyDialog(hdlg_sub);
		hdlg_sub = 0;
		
		dlg_FreeDialog(pdlg_sub);
		pdlg_sub = 0;
	}
	
	//active programming dialog
	dlg_prog_time = 0;
	pdlg_sub = dlg_CreateProgDialog();
	if(pdlg_sub) {
		hdlg_sub = osd_ConstructDialog(pdlg_sub);
		osd_SetActiveDialog(hdlg_sub);
	}
	
	//start programming
	hvp_prog(0, 0);
	return 0;
}

void dlg_init(void)
{
	//set key map
	key_SetLocalKeymap(keymap);
	
	pdlg_main = 0;
	pdlg_sub = 0;
	hdlg_main = 0;
	hdlg_sub = 0;
	dlg_ActiveMainDialog(0);
}
