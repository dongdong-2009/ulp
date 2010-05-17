/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd.h"
#include "osd/osd_eng.h"
#include "osd/osd_event.h"
#include <stdlib.h>

//osd kernel data object, RAM
typedef struct osd_item_ks {
	const osd_item_t *item;
	struct osd_item_ks *next;
} osd_item_k;

typedef struct osd_group_ks {
	const osd_group_t *grp;
	osd_item_k *runtime_kitems;
	short status;
	short focus;
	struct osd_group_ks *next;
	struct osd_group_ks *prev;
} osd_group_k;

typedef struct {
	const osd_dialog_t *dlg;
	osd_group_k *kgrps; //groups currently on screen
	osd_group_k *active_kgrp;
} osd_dialog_k;

//dialog ops
static int osd_ShowDialog(osd_dialog_k *kdlg, int update);
static int osd_HideDialog(osd_dialog_k *kdlg);
static int osd_HandleCommand(int event, const osd_command_t *cmds);

//group ops
static int osd_ConstructGroup(const osd_group_t *grp);
static int osd_DestroyGroup(osd_group_k *kgrp);
static int osd_ShowGroup(osd_group_k *kgrp, int update);
static int osd_HideGroup(osd_group_k *kgrp);

//item ops
static int osd_ConstructItem(const osd_item_t *item);
static int osd_DestroyItem(osd_item_k *kitem);
static int osd_ShowItem(const osd_item_t *item, int status);
static int osd_HideItem(const osd_item_t *item);

static osd_dialog_k *osd_active_kdlg;

void osd_Init(void)
{
	osd_eng_init();
	osd_event_init();
	osd_active_kdlg = NULL;
}

void osd_Update(void)
{
	osd_dialog_k *kdlg;
	osd_group_k *kgrp;
	int event, ret = -1;
	int update = ITEM_UPDATE_ALWAYS;
	
	kdlg = osd_active_kdlg;
	if(kdlg == NULL)
		return;
	
	event = osd_event_get();
	kgrp = kdlg->active_kgrp;
	
	if(event != KEY_NONE) {
		update = ITEM_UPDATE_AFTERCOMMAND;
		if(kgrp != NULL)
			ret = osd_HandleCommand(event, kgrp->grp->cmds);
		if(ret)
			osd_HandleCommand(event, kdlg->dlg->cmds);
	}
	
	osd_ShowDialog(kdlg, update);
}

static int osd_HandleCommand(int event, const osd_command_t *cmds)
{
	if(cmds == NULL)
		return -1;
	
	while(cmds->func != NULL) {
		if(event == cmds->event)
			return cmds->func(cmds);
		cmds ++;
	}
	
	return -1;
}

int osd_SetActiveDialog(int handle)
{
	osd_dialog_k *kdlg = (osd_dialog_k *)handle;
	
	if(kdlg == NULL) {
		return 0;
	}
	
	osd_active_kdlg = kdlg;
	osd_SelectNextGroup();
	return handle;
}

/*
*	success return the new focused group handle
*	else return 0
*	note: focus order is not used yet
*/
int osd_SelectNextGroup(void) //change focus
{
	osd_dialog_k *kdlg = osd_active_kdlg;
	osd_group_k *kgrp, *kgrp_old;
	int flag = 0;
	
	if(kdlg == NULL)
		return 0;
	
	//find next and set focus tag
	kgrp_old = kdlg->active_kgrp;
	kgrp = (kgrp_old == NULL) ? kdlg->kgrps : kgrp_old->next;
	for(; kgrp != NULL; kgrp = kgrp->next)
	{
		if(kgrp->status >= STATUS_NORMAL) {
			//found a new one
			kgrp->focus = 1;
			kdlg->active_kgrp = kgrp;
			if(kgrp_old != NULL)
				kgrp_old->focus = 0;
			flag = 1;
			break;
		}
	}
	
	if(!flag)
		return 0;
	
	flag = osd_eng_scroll(-1, -1);
	if(flag) {
		//hardware scroll is not supported, redraw all
		osd_HideDialog(kdlg);
		osd_eng_scroll(0, kgrp->grp->items->y);
		osd_ShowDialog(kdlg, ITEM_UPDATE_AFTERFOCUSCHANGE);
	}
	return 0;
}

int osd_SelectPrevGroup(void) //change focus
{
	osd_dialog_k *kdlg = osd_active_kdlg;
	osd_group_k *kgrp, *kgrp_old;
	int flag = 0;
	
	if(kdlg == NULL)
		return 0;
	
	kgrp_old = (kdlg->active_kgrp == NULL) ? kdlg->kgrps : kdlg->active_kgrp;
	
	//find next and set focus tag
	for(kgrp = kgrp_old->prev; kgrp != NULL; kgrp = kgrp->prev)
	{
		if(kgrp->status >= STATUS_NORMAL) {
			//found a new one
			kgrp->focus = 1;
			kdlg->active_kgrp = kgrp;
			kgrp_old->focus = 0;
			flag = 1;
			break;
		}
	}
	
	if(!flag)
		return 0;
	
	flag = osd_eng_scroll(-1, -1);
	if(flag) {
		//hardware scroll is not supported, redraw all
		osd_HideDialog(kdlg);
		osd_eng_scroll(0, kgrp->grp->items->y);
		osd_ShowDialog(kdlg, ITEM_UPDATE_AFTERFOCUSCHANGE);
	}
	
	return 0;
}

//construct/destroy/show/hide dialog ops
int osd_ConstructDialog(const osd_dialog_t *dlg)
{
	osd_dialog_k *kdlg;
	osd_group_k *kgrp, *k;
	const osd_group_t *grp;
	int handle;
	
	//construct dialog in memory
	kdlg = malloc(sizeof(osd_dialog_k));
	kdlg->dlg = dlg;
	kdlg->kgrps = NULL;
	kdlg->active_kgrp = NULL;

	for(grp = dlg->grps; (grp->items != NULL)||(grp->cmds != NULL); grp ++)
	{
		handle = osd_ConstructGroup(grp);
		if(handle > 0) {
			kgrp = (osd_group_k *)handle;
			
			//add the kgrp to kdlg alive list
			if(kdlg->kgrps == NULL) {
				kdlg->kgrps = kgrp;
			}
			else {
				k->next = kgrp;
				kgrp->prev = k;
			}

			k = kgrp;
		}
	}

	osd_ShowDialog(kdlg, ITEM_UPDATE_NEVER);
	return (int)kdlg;
}

int osd_DestroyDialog(int handle)
{
	osd_dialog_k *kdlg = (osd_dialog_k *)handle;
	osd_group_k *k, *kgrp;
	
	osd_HideDialog(kdlg);
	
	for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = k->next)
	{
		k = kgrp->next;
		osd_DestroyGroup(kgrp);
	}
	
	if(osd_active_kdlg == kdlg)
		osd_active_kdlg = NULL;
	
	free(kdlg);
	return 0;
}

int osd_ShowDialog(osd_dialog_k *kdlg, int update)
{
	osd_group_k *kgrp;
	
	//show groups
	for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = kgrp->next)
		osd_ShowGroup(kgrp, update);
	
	return (int)kdlg;
}

int osd_HideDialog(osd_dialog_k *kdlg)
{
	osd_group_k *kgrp;
	
	for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = kgrp->next)
	{
		osd_HideGroup(kgrp);
	}
	
	return 0;
}

//group construct/destroy/show/hide ops
static int osd_ConstructGroup(const osd_group_t *grp)
{
	osd_group_k *kgrp;
	const osd_item_t *item;
	osd_item_k *kitem, *k;

	kgrp = malloc(sizeof(osd_group_k));
	kgrp->grp = grp;
	kgrp->next = NULL;
	kgrp->prev = NULL;
	kgrp->runtime_kitems = NULL;
	kgrp->status = STATUS_HIDE;
	kgrp->focus = 0;

	//fill runtime_kitems
	for(item = grp->items; (item != NULL) && (item->draw != NULL); item ++)
	{
		k = (osd_item_k *) osd_ConstructItem(item);
		if(k != NULL) {
			if(kgrp->runtime_kitems == NULL) {
				kgrp->runtime_kitems = k;
			}
			else {
				kitem->next = k;
			}
			
			kitem = k;
		}
	}

	return (int)kgrp;
}

static int osd_DestroyGroup(osd_group_k *kgrp)
{
	osd_item_k *kitem, *kitem_next;
	
	for(kitem = kgrp->runtime_kitems; kitem != NULL;)
	{
		kitem_next = kitem->next;
		osd_DestroyItem(kitem);
		kitem = kitem_next;
	}
	free(kgrp);
	return 0;
}

static int osd_ShowGroup(osd_group_k *kgrp, int update)
{
	int status;
	int (*get_status)(const osd_group_t *grp);
	const osd_group_t *grp;
	const osd_item_t *item;
	osd_item_k *kitem;
	
	//evaluate new group status
	grp = kgrp->grp;
	if(grp->option & GROUP_RUNTIME_STATUS == GROUP_RUNTIME_STATUS) {
		get_status = (int (*)(const osd_group_t *)) grp->order;
		status = get_status(grp);
	}
	else
		status = STATUS_NORMAL; //grp->order;

	if(kgrp->focus)
		status = STATUS_FOCUSED;
		
	if(status != kgrp->status) { //all items needs to be redraw
		kgrp->status = status;
		for(item = grp->items; (item != NULL) && (item->draw != NULL); item ++) {
			osd_HideItem(item);
			osd_ShowItem(item, status);
		}
	}
	else {
		for(kitem = kgrp->runtime_kitems; kitem != NULL; kitem = kitem->next) {
			item = kitem->item;
			if(item->update >= update) {
				osd_HideItem(kitem->item);
				osd_ShowItem(kitem->item, status);
			}
		}
	}
	
	return 0;
}

static int osd_HideGroup(osd_group_k *kgrp)
{
	const osd_item_t *item;
	
	kgrp->status = STATUS_HIDE;
	kgrp->focus = 0;
	for(item = kgrp->grp->items; (item != NULL) && (item->draw != NULL); item ++)
		osd_HideItem(item);
			
	return 0;
}

//item construct/destroy/show/hide ops
static int osd_ConstructItem(const osd_item_t *item)
{
	osd_item_k *kitem = NULL;
	
	if(item->runtime) {
		kitem = malloc(sizeof(osd_item_k));
		kitem->item = item;
		kitem->next = NULL;
	}
	
	return (int)kitem;
}

static int osd_DestroyItem(osd_item_k *kitem)
{
	free(kitem);
	return 0;
}

static int osd_ShowItem(const osd_item_t *item, int status)
{
	return item->draw(item, status);
}

static int osd_HideItem(const osd_item_t *item)
{
	return osd_eng_clear_rect(item->x, item->y, item->w, item->h);
}
