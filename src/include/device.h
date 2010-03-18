/*
 * 	miaofng@2010 initial version
 */
#ifndef __DEVICE_H_
#define __DEVICE_H_

typedef struct {
	int (*write_reg)(int reg, int val);
	int (*read_reg)(int reg);
} dev_io_t;

#endif /*__DEVICE_H_*/
