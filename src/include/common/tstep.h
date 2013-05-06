/*
 * 	feng.ni@2013 initial version
 *	1, a step must return pass/fail
 *	2, how to handle loop/branch structure?
 *	3, fail int first step & last step is prohibited!!!
 */

#ifndef __TSTEP_H
#define __TSTEP_H

typedef int(*tstep_func_t)(void);

/* int test(void)
default step when pass: next step(last step -> first step, first step -> next step)
default step when fail: last step(last step -> last step, first step -> first step)
*/
struct tstep_s {
	tstep_func_t test;
	const char *desc;
};

int tstep_execute(const struct tstep_s *steps, int nr_of_steps);
void tstep_next(tstep_func_t test);
int tstep_fail(void); /*return step nr which failed, or 0 when pass*/

#endif	/* __TSTEP_H */
