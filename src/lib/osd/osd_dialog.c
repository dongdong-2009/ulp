/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_dialog.h"
#include "FreeRTOS.h"

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

	osd_ShowDialog(kdlg, ITEM_UPDATE_NEVER);
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
