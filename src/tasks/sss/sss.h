#ifndef __SSS_H_
#define __SSS_H_

enum {
	SSS_CMD_NONE,
	SSS_CMD_QUERY,
	SSS_CMD_LEARN,
	SSS_CMD_LEARN_RESULT,
	SSS_CMD_SELECT,
	SSS_CMD_SAVE,
	SSS_CMD_CLEAR,
};

static inline int sss_GetID(int slot)
{
	return slot << 1;
}
#endif
