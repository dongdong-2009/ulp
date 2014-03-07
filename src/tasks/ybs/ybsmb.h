/*
*
* miaofng@2014-2-6
*
*/

#ifndef __YBSMB_H__
#define __YBSMB_H__

enum {
	YBS_MB_E_OK,
	YBS_MB_E_CFG,
};

enum {
	YBS_CMD_RESET,
	YBS_CMD_SAVE,
	YBS_CMD_UNLOCK,
};

enum {
	REG_CMD, //like
	REG_CAL = REG_CMD + 2,
	REG_SYS = REG_CAL + 16,
};

#endif
