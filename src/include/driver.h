/*
 * 	miaofng@2010 initial version
 */
#ifndef __DRIVER_H_
#define __DRIVER_H_

#include "linux/list.h"
struct driver {
	const char *name;
	struct list_head drv_list; //pointer to next driver
	const void *ops; //open/close/read/write/ioctl
};

int drv_register(const char *name, struct driver *pdrv);
int drv_open(const char *name); //increase reference counter
int drv_close(int handle); //decrease reference counter

#pragma section=".driver" 4
#define driver_init(init) \
	void (*##init##_entry)(void)@".driver" = &##init

void drv_Init(void);

#endif /*__DRIVER_H_*/
