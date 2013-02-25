/*
 * 	king@2013 initial version
 */

#ifndef __TSTEP_H
#define __TSTEP_H

/* int test(void)
default step when pass: next step(last step -> first step, first step -> next step)
default step when fail: last step(last step -> last step, first step -> first step)
*/
struct tstep_s {
	int (*test)(void);
	int (*pass)(void); //step to do when pass, NULL->next step
	int (*fail)(void); //step to do when fail, NULL->last step
};

int tstep_execute(const struct tstep_s *steps, int nr_of_steps);
#endif	/* __TSTEP_H */
