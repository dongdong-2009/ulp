/*
 * 	miaofng@2010 initial version
 */
#ifndef __DEVICE_H_
#define __DEVICE_H_

typedef struct {
	int (*init)(const void *cfg);
	int (*wreg)(int addr, int val);
	int (*rreg)(int addr);
	int (*wbuf)(char *buf, int n);
	int (*rbuf)(char *buf, int n);
} bus_t;

#endif /*__DEVICE_H_*/
