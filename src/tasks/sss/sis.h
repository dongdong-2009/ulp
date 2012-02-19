/*
 *	miaofng@2011 initial version
 *	king@2011 modify
 */
#ifndef __SIS_H_
#define __SIS_H_

#include "dbs.h"
#include "psi5.h"

/*supported sis protocols*/
enum {
	SIS_PROTOCOL_INVALID,
	SIS_PROTOCOL_DBS,
	SIS_PROTOCOL_PSI5,
};

/*sis_sensor_s, 64bytes*/
struct sis_sensor_s {
	char cksum;
	char protocol;
	char name[14];
	union {
		struct dbs_sensor_s dbs;
		struct psi5_sensor_s psi5; // 22 bytes
		char data[48];
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
		printf("	.mode = 0x%02x\n", (dbs->mode) & 0xff);
		printf("	.addr = 0x%02x\n", (dbs->addr) & 0xff);
		printf("	.speed = 0x%02x(%dmps)\n", dbs->speed, (1 << (4 - dbs->speed))*1000);
		for(int i = 0; i < 8; i ++) {
			printf("	.trace[%d] = 0x%02x\n", i, dbs->trace[i] & 0xff);
		}

		printf("\nConfig Info	0x%02x", sis->protocol & 0xff);
		for(int i = 0; i < sizeof(struct dbs_sensor_s); i ++)
			printf(",0x%02x", sis->data[i] & 0xff);
		printf("\n");
	}
	else if(sis->protocol == SIS_PROTOCOL_PSI5) {
		const struct psi5_sensor_s *psi5 = &sis->psi5;
		printf("	protocol = PSI5\n");
		if(psi5->speed == 0x01)
			printf("	speed = 125kbps\n");
		else if(psi5->speed == 0x02)
			printf("	speed = 189kbps\n");

		if(psi5->parity == 0x00)
			printf("	1 bits even parity\n");
		else if(psi5->parity == 0x01)
			printf("	3 bits CRC parity\n");

		printf("	number of initial data = 0x%02x\n", (psi5->init_data_num) & 0xff);
		printf("	repeat = 0x%02x\n", (psi5->init_repeat) & 0xff);
		printf("	number of status data = 0x%02x\n", (psi5->status_data_num) & 0xff);

		for(int i = 0; i < psi5->init_data_num; i ++) {
			if(i & 0x01)
				printf("	data[%d] = 0x%02x\n", i, (psi5->init_data[i / 2] & 0x00f0) >> 4);
			else
				printf("	data[%d] = 0x%02x\n", i, psi5->init_data[i / 2] & 0x000f);
		}

		printf("\nConfig Info	0x%02x", sis->protocol & 0xff);
		for(int i = 0; i < sizeof(struct psi5_sensor_s); i ++)
			printf(",0x%02x", sis->data[i] & 0xff);
		printf("\n");
	}
	printf("}\n");
	return 0;
}
#endif
