/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_LIGHT_H_
#define __NEST_LIGHT_H_

//light control
enum {
	FAIL_ABORT_ON,
	FAIL_ABORT_OFF,

	PASS_CMPLT_ON,
	PASS_CMPLT_OFF,

	RUNNING_ON,
	RUNNING_OFF,

	ALL_ON,
	ALL_OFF,

	FAIL_ABORT_TOGGLE,
	PASS_CMPLT_TOGGLE,
	RUNNING_TOGGLE,
	ALL_TOGGLE,
};

int nest_light(int cmd);
int nest_light_flash(int err_code);
#endif
