/*
 * 	miaofng@2010 initial version
 */
#ifndef __OSD_CMD_H_
#define __OSD_CMD_H_

#include <stddef.h>

typedef struct osd_command_s {
	int event;
	int (*func)(const struct osd_command_s *cmd);
} osd_command_t;

int osd_HandleCommand(int event, const osd_command_t *cmds);

#endif /*__OSD_CMD_H_*/
