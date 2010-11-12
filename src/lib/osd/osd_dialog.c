/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_dialog.h"
#include "osd/osd_event.h"
#include "FreeRTOS.h"
#include <stdlib.h>

static osd_dialog_k *osd_active_kdlg;

int osd_SetActiveDialog(int handle)
{
	osd_dialog_k *kdlg = (osd_dialog_k *)handle;
	osd_active_kdlg = kdlg;
	if(kdlg == NULL) {
		return 0;
	}
	
	osd_SelectNextGroup();
	return 0;
}

int osd_GetActiveDialog(void)
{
	return (int)osd_active_kdlg;
}

//construct/destroy/show/hide dialog ops
int osd_ConstructDialog(const osd_dialog_t *dlg)
{
	osd_dialog_k *kdlg;
	osd_group_k *kgrp, *k;
	const osd_group_t *grp;
	int handle;
	
	//construct dialog in memory
	kdlg = MALLOC(sizeof(osd_dialog_k));
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

	osd_dlg_get_rect(kdlg, &kdlg->margin);
	return (int)kdlg;
}

int osd_DestroyDialog(int handle)
{
	osd_dialog_k *kdlg = (osd_dialog_k *)handle;
	osd_group_k *k, *kgrp;
	
	osd_HideDialog(kdlg);
	
	for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = k)
	{
		k = kgrp->next;
		osd_DestroyGroup(kgrp);
	}
	
	if(osd_active_kdlg == kdlg)
		osd_active_kdlg = NULL;
	
	FREE(kdlg);
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

rect_t *osd_dlg_get_rect(const osd_dialog_k *kdlg, rect_t *margin)
{
	rect_t r;
	osd_group_k *kgrp;
	
	rect_zero(margin);
	for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = kgrp->next) {
		osd_grp_get_rect(kgrp, &r);
		rect_merge(margin, &r);
	}
	return margin;
}

#ifdef CONFIG_DRIVER_PD
int osd_dlg_react(osd_dialog_k *kdlg, int event, const dot_t *p)
{
	int event = OSDE_NONE;
	osd_group_k *kgrp;
	
	//event occurs outside my dialog?
	if(!rect_have(&kdlg->margin, p))
		return event;
	
	//event occurs outside active group?
	if(rect_have(&kdlg->active_kgrp->margin, p)) {
		event = osd_grp_react(kdlg->active_kgrp, event, p);
	}
	else {
		//change focus
		for(kgrp = kdlg->kgrps; kgrp != NULL; kgrp = kgrp->next) {
#ifdef CONFIG_FONT_TNR08X16
			rect_zoom(&kgrp->margin, 3, 4);
#else
			rect_zoom(&kgrp->margin, 4, 5);
#endif
			if(rect_have(&kgrp->margin, p))
				break;
		}
		
		if(kgrp != NULL) {
			if(!osd_grp_select(kdlg, kgrp))
				event = OSDE_FOCUS;
		}
	}
	
	return event;
}
#endif
