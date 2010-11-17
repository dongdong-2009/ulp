/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_item.h"
#include "osd/osd_eng.h"
#include "osd/osd_event.h"
#include "FreeRTOS.h"
#include <stdlib.h>

int osd_ShowItem(const osd_item_t *item, int status)
{
	return item->draw->draw(item, status);
}

int osd_HideItem(const osd_item_t *item)
{
	return osd_eng_clear_rect(item->x, item->y, item->w, item->h);
}

#ifdef CONFIG_DRIVER_PD
int osd_item_react(const osd_item_t *item, int event, const dot_t *p)
{
	if(item->draw->react != NULL)
		event = item->draw->react(item, event, p);
	else event = OSDE_NONE;

	return event;
}
#endif

