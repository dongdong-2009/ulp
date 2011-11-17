/*
 * 	miaofng@2010 initial version
 *	miaofng@2011 implement
 *	to identify which driver belongs to wich device, the name of driver and device must be the same
 */
#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "ulp/driver.h"
#include "linux/list.h"

struct device {
	const char *name;
	struct list_head list;
	struct driver *pdrv;
};

/*path is something like 'uart0'(class name + index) or 'uart_stm32_0'(device name)*/
int dev_open(const char *path, const char *mode);
int dev_ioctl(int handle, int request, ...);
int dev_poll(int handle, int event);
int dev_read(void *ptr, int size, int count, int handle);
int dev_write(const void *ptr, int size, int count, int handle);
int dev_close(int handle);

int dev_register(const char *name, struct device *pdev);

#endif /*__DEVICE_H_*/
