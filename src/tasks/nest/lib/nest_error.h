/*
 * 	miaofng@2011 initial version
 */

#ifndef __NEST_ERROR_H_
#define __NEST_ERROR_H_

enum {
	NO_FAIL = 0,
	PULSE_FAIL = 1,
	RAM_FAIL	= 2,
	FB_FAIL = 3,
	FIS_FAIL = 4,
	CAN_FAIL = 5,
	PSV_FAIL = 6,
	MISC_FAIL = 7,
	NEC_FAIL = 7, //SET "NEST ERROR CODE" FAIL
	MTYPE_FAIL = 7,
	SN_FAIL = 7,
	DUMMY
};

struct nest_error_s {
	int type;
	int time; //unit ms
	const char *info; //error info
};

#define nest_pass() (nest_error_get() -> type == NO_FAIL)
#define nest_fail() (nest_error_get() -> type != NO_FAIL)

int nest_error_init(void);
struct nest_error_s* nest_error_get(void);
int nest_error_set(int type, const char *info);

#endif
