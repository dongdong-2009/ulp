/* osd.h
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_DIALOG_H_
#define __OSD_DIALOG_H_

#include "osd/osd_cmd.h"
#include "osd/osd_group.h"

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

//private, osd kernel data object, RAM
typedef struct {
	const osd_dialog_t *dlg;
	osd_group_k *kgrps; //groups currently on screen
	osd_group_k *active_kgrp;
} osd_dialog_k;

//public api
int osd_ConstructDialog(const osd_dialog_t *dlg);
int osd_SetActiveDialog(int handle); //enable cmd handling
int osd_GetActiveDialog(void);
int osd_DestroyDialog(int handle);

//private
int osd_ShowDialog(osd_dialog_k *kdlg, int update);
int osd_HideDialog(osd_dialog_k *kdlg);

#endif /*__OSD_DIALOG_H_*/
