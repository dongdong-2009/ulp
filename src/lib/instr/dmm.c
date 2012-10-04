/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "instr/instr.h"
#include "instr/dmm.h"
#include "crc.h"

static struct dmm_s *dmm = NULL;

struct dmm_s* dmm_open(const char *name)
{
	struct instr_s *instr = instr_open(INSTR_CLASS_DMM, name);
	if(instr == NULL) {
		return NULL;
	}

	struct dmm_s *dmm_new = sys_malloc(sizeof(struct dmm_s));
	sys_assert(dmm_new != NULL);
	dmm_new->instr = instr;

	dmm_select(dmm_new);
	return dmm_new;
}

void dmm_close(void)
{
	instr_close(dmm->instr);
	sys_free(dmm);
	dmm = NULL;
}

int dmm_select(struct dmm_s *dmm_new)
{
	dmm = dmm_new;
	return 0;
}

int dmm_read(int ch)
{
	return 0;
}
