/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_ERROR_H_
#define __NEST_ERROR_H_

enum {
	NO_FAIL = 0,
	CAN_FAIL = 1, //communication fail
	MTYPE_FAIL = 2, //model type cannot identified
	PSV_FAIL = 3, //process not ready

	RAM_FAIL = 4, //ram test
	BURN_FAIL = 5,
	FB_FAIL = 6,

	EEWRITE_FAIL = 7,

	//obsoleted, pls do not use them any more
	FIS_FAIL = 5, //fis access fail
	PULSE_FAIL = 7,
	MISC_FAIL = 7,
	NEC_FAIL = 7, //SET "NEST ERROR CODE" FAIL
	SN_FAIL = 7,
	DUMMY
};

struct nest_error_s {
	int type;
	int time; //unit ms
	const char *info; //error info

	char nec;
};

#define nest_pass() (nest_error_get() -> type == NO_FAIL)
#define nest_fail() (nest_error_get() -> type != NO_FAIL)

int nest_error_init(void);
struct nest_error_s* nest_error_get(void);
int nest_error_set(int type, const char *info);

#endif
