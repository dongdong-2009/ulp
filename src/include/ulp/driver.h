/*
 * 	miaofng@2010 initial version
 */
#ifndef __DRIVER_H_
#define __DRIVER_H_

#include "linux/list.h"

struct drv_ops_s {
	int (*init)(int fd, void *pcfg);
	int (*open)(int fd);
	int (*ioctl)(int fd, int request, ...);
	int (*poll)(int fd, int event);
	int (*read)(int fd, void *buf, int count);
	int (*write)(int fd, const void *buf, int count);
	int (*close)(int fd);
};

struct driver_s {
	const char *name;
	struct list_head list; //next drv in all drv list
	const struct drv_ops_s *ops; //init/open/close/read/write/ioctl/poll
};

struct driver_s* drv_register(const char *name, const struct drv_ops_s *ops);
struct driver_s* drv_open(const char *name);
int drv_close(driver_s *pdrv);

#pragma section=".driver" 4
#define driver_init(init) \
	void (*##init##_entry)(void)@".driver" = &##init

void drv_Init(void);

#endif /*__DRIVER_H_*/
