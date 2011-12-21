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

/*do not touch my element directly, because it's very likely to be changed later ...*/
struct device_s {
	const char *name;
	int ref;
	struct list_head list; //next dev in all devs list
	struct list_head lcls; //next dev in the same class
	struct driver_s *pdrv;
	void *priv; //to store device specific data
	const void *pcfg;
};

static inline void* dev_priv_get(int fd) {
	struct device_s * dev = (struct device_s *)fd;
	assert(dev != NULL);
	return dev->priv;
}

static inline int dev_priv_set(int fd, void *priv) {
	struct device_s * dev = (struct device_s *)fd;
	assert(dev != NULL);
	dev->priv = priv;
	return 0;
}

/*dev_poll events*/
#define POLLIN		1
#define POLLOUT		2

/*name is something like 'uart0'(class name + index) or 'uart_stm32.0'(device name)*/
int dev_open(const char *name, const char *mode); //success return handle, fail -1
int dev_ioctl(int fd, int request, ...);
int dev_poll(int fd, int event);
int dev_read(int fd, void *buf, int count);
int dev_write(int fd, const void *buf, int count);
int dev_close(int fd);

//match the driver automatically
int dev_register(const char *name, const void *pcfg); //success return 0
int dev_class_register(const char *name, int fd); //success return 0

//auto scan new device & probe its driver
int dev_probe(void);

#endif /*__DEVICE_H_*/
