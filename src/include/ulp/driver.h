/*
 * 	miaofng@2010 initial version
 */
#ifndef __DRIVER_H_
#define __DRIVER_H_

#include "linux/list.h"
#include <stdarg.h>

struct drv_ops_s {
	/*automatically called when driver matches an device,
	pls do not call dev_open() inside this function, because
	all device list is incomplete yet*/
	int (*init)(int fd, const void *pcfg);

	int (*open)(int fd, const void *pcfg);
	int (*ioctl)(int fd, int request, va_list args);
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

/*return 0 indicates success*/
int drv_register(const char *name, const struct drv_ops_s *ops);
#pragma section=".driver" 4
#define driver_init(init) \
	void (*##init##_entry)(void)@".driver" = &##init

/*private*/
struct driver_s* drv_open(const char *name);
int drv_close(struct driver_s *pdrv);
void drv_Init(void);

#endif /*__DRIVER_H_*/
