/*
 * 	miaofng@2010 initial version
 *	miaofng@2011 implement
 *	to identify which driver belongs to wich device, the name of driver and device must be the same
 */
#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "ulp/driver.h"
#include "ulp/debug.h"
#include "linux/list.h"
#include <stdlib.h>

struct device_s {
	const char *name;
	char suffix; //uart_stm32.1 uart_stm32.2 uart_sam3u.g
	struct list_head list; //next dev in all devs list
	struct list_head lcls; //next dev in the same class
	struct driver *pdrv;
	void *priv; //to store device specific data
	const void *pcfg;
};

static inline void* dev_priv_get(int fd) {
	struct device_s * dev = (struct device_s *)fd;
	assert(dev != NULL);
	return dev->priv;
}

static inline int dev_priv_set(int fd, const void *priv) {
	struct device_s * dev = (struct device_s *)fd;
	assert(dev != NULL);
	fd->priv = priv;
	return 0;
}

/*path is something like 'uart0'(class name + index) or 'uart_stm32.0'(device name)*/
int dev_open(const char *path, const char *mode);
int dev_ioctl(int fd, int request, ...);
int dev_poll(int fd, int event);
int dev_read(int fd, void *buf, int count);
int dev_write(int fd, const void *buf, int count);
int dev_close(int fd);

struct device_s *__dev_get(const char *path);
int __dev_init(struct device_s *pdev);
int dev_register(const char *path, const void *pcfg);

/*handling device class*/
struct class_s {
	const char *name;
	struct list_head list; //next class in all class list
	struct list_head devs; //head of devs of this class
}

int class_dev_add(const char *name, struct device_s *pdev);
struct device_s* class_dev_get(const char *name);

#endif /*__DEVICE_H_*/
