/*
 *  miaofng@2012 initial version
 */

#ifndef __INSTR_DMM_H_
#define __INSTR_DMM_H_

#include "instr/instr.h"

enum {
	DMM_PROFILE_R = 'R',
	DMM_PROFILE_V = 'V',
};

struct dmm_s {
	char profile;
	struct instr_s *instr;
};

enum {
	DMM_CMD_CONFIG = INSTR_CMD_END,
	DMM_CMD_READ,
};

/* each instrument can only be opened once
 * name  could be 'dmm0'(class_name + index), xxx(uuid),
 *       or yyy[:0](name[:index]), or null
 */
struct dmm_s* dmm_open(const char *name);
int dmm_select(struct dmm_s *dmm_new);
int dmm_config(const char *str_cfg);
int dmm_read(void);
void dmm_close(void);

#endif /* __INSTR_DMM_H_ */
