#ifndef __SSS_H_
#define __SSS_H_

enum {
	SSS_CMD_NONE,
	SSS_CMD_QUERY,
	SSS_CMD_LEARN,
	SSS_CMD_SELECT,
};

static inline int sss_GetID(int slot)
{
	return slot << 1;
}
#endif
