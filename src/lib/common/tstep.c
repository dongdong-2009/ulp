/*
 * 	miaofng@2013-2-22 initial version
 */

#include "ulp/sys.h"
#include "common/tstep.h"

static int tstep_search(int (*test)(void), const struct tstep_s *steps, int nr_of_steps)
{
	int idx = -1;
	for(int i = 0; i < nr_of_steps; i ++) {
		int (*pfunc)(void);
		pfunc = steps[i].test;
		if(pfunc == test) {
			idx = i;
			break;
		}
	}
	return idx;
}

int tstep_execute(const struct tstep_s *steps, int nr_of_steps)
{
	int idx, ecode;
	int (*test)(void);

	sys_assert((steps != NULL) && (nr_of_steps > 0));
	test = steps[0].test;
	while(1) {
		ecode = test();
		idx = tstep_search(test, steps, nr_of_steps);
		sys_assert(idx >= 0);
		switch(ecode) {
		case 0:
			idx ++;
			idx = (idx == nr_of_steps) ? 0 : idx;
			test = steps[idx].test;
			break;
		case 1:
			idx = (idx == 0) ? 0 : nr_of_steps - 1;
			test = steps[idx].test;
			break;
		default:
			test = (int (*)(void)) ecode;
			idx = tstep_search(test, steps, nr_of_steps);
			sys_assert(idx >= 0);
		}
	}
}
