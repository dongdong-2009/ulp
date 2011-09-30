/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_GROUP_H_
#define __OSD_GROUP_H_

#include "osd/osd_cmd.h"
#include "osd/osd_item.h"
#include "common/glib.h"

//group option
enum {
	GROUP_RUNTIME_STATUS = 1 << 0, //indicates status is a func
	GROUP_DRAW_FULLSCREEN = 1 << 1, //indicates this group takes up whole screen
};

//group status
enum {
	STATUS_HIDE = -100,
	STATUS_GRAYED,
	STATUS_VISIBLE,
	STATUS_FOCUSED,
};

typedef struct osd_group_s {
	const osd_item_t *items;
	const osd_command_t *cmds;
	int order; //group status or a status func
	int option;
} osd_group_t;

//private
#define COLOR_FG_DEF BLACK
#define COLOR_BG_DEF WHITE	
#define COLOR_FG_FOCUS WHITE
#define COLOR_BG_FOCUS BLUE

struct osd_dialog_ks;
typedef struct osd_group_ks {
	const osd_group_t *grp;
	short status;
	short focus;
	struct osd_group_ks *next;
	struct osd_group_ks *prev;
	rect_t margin;
} osd_group_k;

//api
int osd_SelectGroup(const osd_group_t *grp);
int osd_SelectNextGroup(void); //change focus
int osd_SelectPrevGroup(void); //change focus
osd_group_t *osd_GetCurrentGroup(void);

//private
int osd_ConstructGroup(const osd_group_t *grp);
int osd_DestroyGroup(osd_group_k *kgrp);
int osd_ShowGroup(osd_group_k *kgrp, int ops);
int osd_HideGroup(osd_group_k *kgrp);
int osd_grp_select(struct osd_dialog_ks *kdlg, osd_group_k *kgrp);
int osd_grp_get_status(const osd_group_k *grp, int ops);
rect_t *osd_grp_get_rect(const osd_group_k *kgrp, rect_t *margin);
#ifdef CONFIG_DRIVER_PD
int osd_grp_react(osd_group_k *kgrp, int event, const dot_t *p);
#endif
#endif /*__OSD_GROUP_H_*/
