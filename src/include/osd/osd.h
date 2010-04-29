/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_H_
#define __OSD_H_

#include <stddef.h>
#include "osd/item.h"

#define ITEM_DRAW_TXT	item_DrawTxt
#define ITEM_DRAW_INT	item_DrawInt

typedef struct osd_command_s {
	int event;
	int (*func)(const struct osd_command_s *cmd);
} osd_command_t;

//group option
enum {
	GROUP_RUNTIME_STATUS = 1 << 0, //indicates status is a func
	GROUP_DRAW_FULLSCREEN = 1 << 1, //indicates this group takes up whole screen
};

typedef struct osd_group_s {
	const osd_item_t *items;
	const osd_command_t *cmds;
	int order; //focus order or group status or a status func, refer to item/group status in item.h
	int option;
} osd_group_t;

//dialog option
enum {
	DIALOG_DRAW_FULLSCREEN = 1 << 0, //indicates this dialog takes up the whole screen
};

typedef struct osd_dialog_s {
	const osd_group_t *grps;
	const osd_command_t *cmds;
	int (*func)(const struct osd_dialog_s *dlg, int status); //init/close ...
	int option; //dialog option
} osd_dialog_t;

void osd_Init(void);
void osd_Update(void);
int osd_ConstructDialog(const osd_dialog_t *dlg);
int osd_SetActiveDialog(int handle); //enable cmd handling
int osd_DestroyDialog(int handle);
int osd_SelectNextGroup(void); //change focus
int osd_SelectPrevGroup(void); //change focus
#endif /*__OSD_H_*/
