/*
 * 	miaofng@2010 initial version
 *		- stores all osd_xx_t object in flash
 *		- setup a fast link in memory for all dialog, all group and runtime items inside that group
 *		- do not construct several dialog at the same time unless they takes up different osd region
 */

#include "config.h"
#include "osd/osd_cmd.h"

int osd_HandleCommand(int event, const osd_command_t *cmds)
{
	int handled = -1;
	
	if(cmds == NULL)
		return handled;
	
	while(cmds->func != NULL) {
		if(event == cmds->event) {
			cmds->func(cmds);
			handled = 0;
		}
		cmds ++;
	}
	
	return handled;
}