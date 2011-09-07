/*
 *	miaofng@2011 initial version
 */
#ifndef __SIS_H_
#define __SIS_H_

#include "dbs.h"

/*supported sis protocols*/
enum {
	SIS_PROTOCOL_INVALID,
	SIS_PROTOCOL_DBS,
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

static inline char sis_sum(void *p, int n)
{
	char sum, *str = (char *) p;
	for(sum = 0; n > 0; n --) {
		sum += *str;
		str ++;
	}
	return sum;
}

static inline char sis_print(const struct sis_sensor_s *sis)
{
	printf("sensor %s = {\n", sis->name);
	if(sis->protocol == SIS_PROTOCOL_DBS) {
		const struct dbs_sensor_s *dbs = &sis->dbs;
		printf("	.speed = 0x%02x(%dmps)\n", dbs->speed, (1 << (4 - dbs->speed))*1000);
		printf("	.mode = 0x%02x\n", dbs->mode);
		printf("	.addr = 0x%02x\n", dbs->addr);
		for(int i = 0; i < 8; i ++) {
			unsigned x = dbs->trace[i] & 0xff;
			printf("	.trace[%d] = 0x%02x\n", i, x);
		}
	}
	printf("}\n");
	return 0;
}
#endif
