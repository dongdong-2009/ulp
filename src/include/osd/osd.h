/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_H_
#define __OSD_H_

#include <stddef.h>
#include "osd/item.h"

#define ITEM_DRAW_TXT	item_DrawTxt
#define ITEM_DRAW_INT	item_DrawInt

typedef osd_commands_s {
	int cmd;
	const int (*func)(const osd_commands_s *cmd);
} osd_commands_t;

//group status
enum {
	GROUP_STATUS_NOTPRESENT = -3,
	GROUP_STATUS_ERASE = -2,
	GROUP_STATUS_VISIBLE = -1,
};

//group option
enum {
	GROUP_RUNTIME_STATUS, //indicates status is a func
};

typedef struct osd_group_s {
	const osd_item_t *items[];
	const osd_commands_t *cmds[];
	int status; //focus order or group status or a status func, refer to group status
	int option;
} osd_group_t;

typedef osd_dialog_s {
	const osd_group_t *grps[];
	const osd_commands_t *cmds[];
	const int (*func)(const struct osd_dialog_s *dlg, int status); //init/close ...
	int grps_max; //max scrollable groups
} osd_dialog_t;

void osd_Init(void);
void osd_Update(void);
int osd_ConstructDialog(const osd_dialog_t *dlg);
int osd_SetActiveDialog(int handle); //enable cmd handling
int osd_SelectNextGroup(void); //change focus up/dn
int osd_SelectPrevGroup(void);
#endif /*__OSD_H_*/
