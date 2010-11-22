/*
*	this file tries to provide all common nest ops here
 * 	miaofng@2010 initial version
 */

#ifndef __NEST_H_
#define __NEST_H_

#include "config.h"
#include "time.h"
#include "cncb.h"
#include "priv/mcamos.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int nest_init(void);
int nest_update(void);

//message handling
#define nest_message printf

//time handling
void nest_time_init(void);
int nest_time_get(void);

//error handling
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

int nest_error_init(void);
struct nest_error_s* nest_error_get(void);
int nest_error_set(int type, const char *info);
#define nest_pass() (nest_error_get() -> type == NO_FAIL)
#define nest_fail() (nest_error_get() -> type != NO_FAIL)

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
int nest_light_flash(int code);

//nest maintain
struct nest_info_s {
	int id_base;
	int id_fixture;
};

struct nest_info_s* nest_info_get(void);
#endif