/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_item.h"
#include "osd/osd_eng.h"
#include "FreeRTOS.h"

//item construct/destroy/show/hide ops
int osd_ConstructItem(const osd_item_t *item)
{
	osd_item_k *kitem = NULL;
	
	if(item->runtime || item->update == ITEM_UPDATE_ALWAYS) {
		kitem = MALLOC(sizeof(osd_item_k));
		kitem->item = item;
		kitem->next = NULL;
	}
	
	return (int)kitem;
}

int osd_DestroyItem(osd_item_k *kitem)
{
	FREE(kitem);
	return 0;
}

int osd_ShowItem(const osd_item_t *item, int status)
{
	return item->draw(item, status);
}

int osd_HideItem(const osd_item_t *item)
{
	return osd_eng_clear_rect(item->x, item->y, item->w, item->h);
}
