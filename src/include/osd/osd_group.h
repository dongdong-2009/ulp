/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_GROUP_H_
#define __OSD_GROUP_H_

#include "osd/osd_cmd.h"
#include "osd/osd_item.h"

//group option
enum {
	GROUP_RUNTIME_STATUS = 1 << 0, //indicates status is a func
	GROUP_DRAW_FULLSCREEN = 1 << 1, //indicates this group takes up whole screen
};

//group status
enum { 
	STATUS_HIDE = -2,
	STATUS_VISIBLE,
	STATUS_GRAYED,
	STATUS_NORMAL,
	STATUS_FOCUSED,
};

typedef struct osd_group_s {
	const osd_item_t *items;
	const osd_command_t *cmds;
	int order; //focus order or group status or a status func, refer to item/group status in item.h
	int option;
} osd_group_t;

//private
#define COLOR_FG_DEF BLACK
#define COLOR_BG_DEF WHITE	
#define COLOR_FG_FOCUS WHITE
#define COLOR_BG_FOCUS BLUE

typedef struct osd_group_ks {
	const osd_group_t *grp;
	osd_item_k *runtime_kitems;
	short status;
	short focus;
	struct osd_group_ks *next;
	struct osd_group_ks *prev;
} osd_group_k;

//api
int osd_SelectNextGroup(void); //change focus
int osd_SelectPrevGroup(void); //change focus
osd_group_t *osd_GetCurrentGroup(void);

//private
int osd_ConstructGroup(const osd_group_t *grp);
int osd_DestroyGroup(osd_group_k *kgrp);
int osd_ShowGroup(osd_group_k *kgrp, int update);
int osd_HideGroup(osd_group_k *kgrp);

#endif /*__OSD_GROUP_H_*/
