/*
 * 	miaofng@2013-2-22 initial version
 */

#include "ulp/sys.h"
#include "common/tstep.h"
#include "common/ansi.h"

static int tstep_estep;
static tstep_func_t tstep_next_func = NULL;

static int tstep_search(tstep_func_t test, const struct tstep_s *steps, int nr_of_steps)
{
	int idx = -1;
	for(int i = 0; i < nr_of_steps; i ++) {
		tstep_func_t pfunc;
		pfunc = steps[i].test;
		if(pfunc == test) {
			idx = i;
			break;
		}
	}
	return idx;
}

void tstep_next(tstep_func_t test)
{
	tstep_next_func = test;
}

int tstep_fail(void)
{
	return tstep_estep;
}

int tstep_execute(const struct tstep_s *steps, int nr_of_steps)
{
	int idx, ecode;
	tstep_func_t test;

	sys_assert((steps != NULL) && (nr_of_steps >= 2));
	test = steps[0].test;

	while(1) {
		idx = tstep_search(test, steps, nr_of_steps);
		sys_assert(idx >= 0);

		if(idx == 0) {
			tstep_estep = 0;
			printf(ANSI_ERASE_SCREEN);
			printf("\n");
		}

		const char *desc = steps[idx].desc;
		printf(ANSI_FONT_GREEN);
		printf("STEP%d: %s ..\n", idx, (desc == NULL) ? "" : desc);
		printf(ANSI_FONT_DEF);
		tstep_next_func = NULL;
		ecode = test();
		if(ecode == 0) {
			if(tstep_next_func != NULL) {
				test = tstep_next_func;
			}
			else {
				idx ++;
				idx = (idx == nr_of_steps) ? 0 : idx;
				test = steps[idx].test;
			}
		}
		else {
			printf(ANSI_FONT_GREEN);
			printf("STEP%d: FAIL !!!\n", idx);
			printf(ANSI_FONT_DEF);
			tstep_estep = idx;
			idx = (idx == 0) ? 0 : nr_of_steps - 1;
			test = steps[idx].test;
		}
	}
}
