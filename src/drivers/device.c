/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "ulp/device.h"
#include "sys/malloc.h"
#include "linux/list.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static LIST_HEAD(class_queue);

int class_dev_add(const char *name, struct device_s *pdev)
{
	struct list_head *pos;
	struct class_s *q, *new = NULL;

	//try to find the class with specified name
	list_for_each(pos, &class_queue) {
		q = list_entry(pos, class_s, list);
		if(!strcmp(name, q->name)) {
			new = q;
			break;
		}
	}

	if(new == NULL) { //create in case of a new device class
		new = sys_malloc(sizeof(struct class_s));
		assert(new != NULL);

		new->name = name;
		INIT_LIST_HEAD(&new->list);
		INIT_LIST_HEAD(&new->devs);
	}

	//add the specified device to the class dev list
	assert(pdev != NULL);
	list_add_tail(&pdev->lcls, &new->devs);
	return 0;
}

struct device_s* class_dev_get(const char *name)
{
	struct list_head *pos;
	struct class_s *q, *new = NULL;
	struct device_s *pdev = NULL;
	int n, index;

	assert(name != NULL);
	n = strlen(name) - 1;
	if(!isdigit(name[n])) //name must like uart0 or uart01
		return NULL;

	index = name[n] - '0';
	if(isdigit(name[n - 1])) {
		n --;
		index += (name[n] - '0') * 10;
	}

	//try to find the class with specified name
	list_for_each(pos, &class_queue) {
		q = list_entry(pos, class_s, list);
		if(!strncmp(name, q->name, n)) {
			new = q;
			break;
		}
	}

	if(new == NULL)
		return NULL;

	//search the dev in the class
	n = 0;
	list_for_each(pos, &new->devs) {
		if(n == index) {
			pdev = list_entry(pos, device_s, lcls);
			break;
		}
	}

	return pdev;
}

int dev_open(const char *path, const char *mode)
{	
}

int dev_ioctl(int fd, int request, ...)
{
}

int dev_poll(int fd, int event)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	
	assert(dev != NULL);
	if(dev->driver != NULL && dev->driver->ops.poll != NULL) {
		ret = (*(dev->driver->ops.poll))(fd, event);
	}
	return ret;
}

int dev_read(int fd, void *buf, int count)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	
	assert(dev != NULL);
	if(dev->driver != NULL && dev->driver->ops.read != NULL) {
		ret = (*(dev->driver->ops.read))(fd, buf, count);
	}
	return ret;
}

int dev_write(int fd, const void *buf, int count)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	
	assert(dev != NULL);
	if(dev->driver != NULL && dev->driver->ops.write != NULL) {
		ret = (*(dev->driver->ops.poll))(fd, buf, count);
	}
	return ret;
}

int dev_close(int fd)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	
	assert(dev != NULL);
	if(dev->driver != NULL && dev->driver->ops.close != NULL) {
		ret = (*(dev->driver->ops.close))(fd);
	}
	return ret;
}

int dev_register(const char *path, void *pcfg)
{
}
