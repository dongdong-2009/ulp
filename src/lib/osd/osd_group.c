/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_group.h"
#include "osd/osd_dialog.h"
#include "osd/osd_eng.h"
#include "FreeRTOS.h"
#include <stdlib.h>

/*
*	success return the new focused group handle
*	else return 0
*	note: focus order is not used yet
*/
int osd_SelectNextGroup(void) //change focus
{
	osd_dialog_k *kdlg = (osd_dialog_k *) osd_GetActiveDialog();
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
		if(osd_eng_is_visible(0, kgrp->grp->items->y))
			osd_eng_scroll(0, kgrp->grp->items->y);
		osd_ShowDialog(kdlg, ITEM_UPDATE_AFTERFOCUSCHANGE);
	}
	return 0;
}

int osd_SelectPrevGroup(void) //change focus
{
	osd_dialog_k *kdlg = (osd_dialog_k *) osd_GetActiveDialog();
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
		if(osd_eng_is_visible(0, kgrp->grp->items->y))
			osd_eng_scroll(0, kgrp->grp->items->y);
		osd_ShowDialog(kdlg, ITEM_UPDATE_AFTERFOCUSCHANGE);
	}
	
	return 0;
}

osd_group_t *osd_GetCurrentGroup(void)
{
	const osd_group_t *grp = NULL;
	osd_dialog_k *kdlg = (osd_dialog_k *) osd_GetActiveDialog();
	if(kdlg) {
		grp = kdlg->active_kgrp->grp;
	}
	
	return (osd_group_t *)grp;
}

//group construct/destroy/show/hide ops
int osd_ConstructGroup(const osd_group_t *grp)
{
	osd_group_k *kgrp;
	const osd_item_t *item;
	osd_item_k *kitem, *k;

	kgrp = MALLOC(sizeof(osd_group_k));
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

int osd_DestroyGroup(osd_group_k *kgrp)
{
	osd_item_k *kitem, *kitem_next;
	
	for(kitem = kgrp->runtime_kitems; kitem != NULL;)
	{
		kitem_next = kitem->next;
		osd_DestroyItem(kitem);
		kitem = kitem_next;
	}
	FREE(kgrp);
	return 0;
}

static int set_color(int status)
{
	int fg, bg;
	
	fg = (int) COLOR_FG_DEF;
	bg = (int) COLOR_BG_DEF;
	
	if(status == STATUS_FOCUSED) {
		fg = (int) COLOR_FG_FOCUS;
		bg = (int) COLOR_BG_FOCUS;
	}
	
	return osd_eng_set_color(fg, bg);
}

int osd_ShowGroup(osd_group_k *kgrp, int update)
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
	
	set_color(status);
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

int osd_HideGroup(osd_group_k *kgrp)
{
	const osd_item_t *item;
	kgrp->status = STATUS_HIDE;
	set_color(kgrp->status);
	//kgrp->focus = 0;
	for(item = kgrp->grp->items; (item != NULL) && (item->draw != NULL); item ++)
		osd_HideItem(item);
			
	return 0;
}
