/*
 *	miaofng@2011 initial version
 */
#ifndef __SIS_H_
#define __SIS_H_

#include "dbs.h"

/*supported sis protocols*/
enum {
	SIS_PROTOCOL_DBS,
	SIS_PROTOCOL_INVALID,
};

/*sis_sensor_s, 32bytes*/
struct sis_sensor_s {
	char cksum;
	char protocol;
	char name[14];
	union {
		struct dbs_sensor_s dbs;
		char data[15];
	};
};

char sis_sum(void *p, int n);
#endif
