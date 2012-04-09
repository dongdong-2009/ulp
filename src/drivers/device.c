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

/*handling device class*/
struct class_s {
	const char *name;
	struct list_head list; //next class in all class list
	struct list_head dev_queue; //head of devs of this class
};

static LIST_HEAD(dev_class_queue);
static LIST_HEAD(dev_queue);

/*private routines*/
static struct device_s* dev_class_get(const char *name);
static struct device_s* dev_get(const char *name);

static struct device_s* dev_get(const char *name)
{
	struct list_head *pos;
	struct device_s *q;

	//search the device with the same name, like uart_stm32 or uart_stm32.1 or uart_sam3u.g
	list_for_each(pos, &dev_queue) {
		q = list_entry(pos, device_s, list);
		if(!strcmp(name, q->name)) {
			return q;
		}
	}

	//not found? "uart_stm32.1" = "uart_stm32" + "1"
	char *p = strchr(name, '.');
	if((p == NULL) || !isdigit(p[1]))
		return NULL;

	//loosely match
	int i, n = p - name;
	int index = atoi(p + 1);

	i = 0;
	list_for_each(pos, &dev_queue) {
		q = list_entry(pos, device_s, list);
		if(!strncmp(name, q->name, n)) {
			if(index == i)
				return q;
			else
				i ++;
		}
	}
	return NULL;
}

int dev_probe(void)
{
	struct list_head *pos;
	struct device_s *q;
	int counter = 0;

	list_for_each(pos, &dev_queue) {
		q = list_entry(pos, device_s, list);
		if(q->pdrv == NULL) {
			q->pdrv = drv_open(q->name);
			counter += (q->pdrv != NULL) ? 1 : 0;
		}
	}
	return counter;
}

//name = "uart_stm32" or "uart_stm32.1" or "uart_sam3u.g" or "uart_sam3u.debug"
//note: only the string part on the left side of the name will be recoganized as the key to match the driver
int dev_register(const char *name, const void *pcfg)
{
	struct device_s *new = NULL;
	int (*init)(int fd, const void *pcfg);

	assert(name != NULL);
	new = sys_malloc(sizeof(struct device_s));
	assert(new != NULL);

	new->name = name;
	new->ref = 0; //open/close counter
	INIT_LIST_HEAD(&new->list);
	INIT_LIST_HEAD(&new->lcls);
	new->pdrv = drv_open(name);
	new->priv = NULL;

	//add the specified device to the all dev list
	list_add_tail(&new->list, &dev_queue);
	if(new->pdrv != NULL && new->pdrv->ops->init != NULL) {
		init = new->pdrv->ops->init;
		return (*init)((int)new, pcfg);
	}
	return 0;
}

int dev_open(const char *name, const char *mode)
{
	int ret;
	struct device_s *dev = dev_class_get(name);
	int (*open)(int fd, int mode);

	if(dev == NULL)
		dev = dev_get(name);

	if(dev == NULL || dev->ref > 0) {
		assert(1 == 0); //dev open fail, for debug purpose
		return 0;
	}

	if(dev->pdrv == NULL) //no driver found
		return 0;

	open = dev->pdrv->ops->open;
	if(open != NULL) {
		ret = (*open)((int)dev, 0);
		if(ret)
			return 0;
	}

	dev->ref += 1;
	return (int)dev;
}

int dev_ioctl(int fd, int request, ...)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	int (*ioctl)(int fd, int request, va_list args);
	va_list args;

	//assert(dev != NULL);
	ioctl = dev->pdrv->ops->ioctl;
	if(ioctl != NULL) {
		va_start(args, request);
		ret = (*ioctl)(fd, request, args);
		va_end(args);
	}
	return ret;
}

int dev_poll(int fd, int event)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	int (*poll)(int fd, int event);

	//assert(dev != NULL);
	poll = dev->pdrv->ops->poll;
	if(poll != NULL) {
		ret = (*poll)(fd, event);
	}
	return ret;
}

int dev_read(int fd, void *buf, int count)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	int (*read)(int fd, void *buf, int count);

	//assert(dev != NULL);
	read = dev->pdrv->ops->read;
	if(read != NULL) {
		ret = (*read)(fd, buf, count);
	}
	return ret;
}

int dev_write(int fd, const void *buf, int count)
{
	int ret = -1;
	struct device_s *dev = (struct device_s *) fd;
	int (*write)(int fd, const void *buf, int count);

	//assert(dev != NULL);
	write = dev->pdrv->ops->write;
	if(write != NULL) {
		ret = (*write)(fd, buf, count);
	}
	return ret;
}

int dev_close(int fd)
{
	int ret = 0;
	struct device_s *dev = (struct device_s *) fd;
	int (*close)(int fd);

	assert(dev != NULL);
	dev->ref -= 1;
	close = dev->pdrv->ops->close;
	if(close != NULL) {
		ret = (*close)(fd);
	}
	return ret;
}

int dev_class_register(const char *name, int fd)
{
	struct list_head *pos;
	struct class_s *q, *new = NULL;
	struct device_s *pdev = (struct device_s *) fd;
	assert(pdev != NULL);

	//try to find the class with specified name
	list_for_each(pos, &dev_class_queue) {
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
		INIT_LIST_HEAD(&new->dev_queue);

		//add the new class to all class list
		list_add_tail(&new->list, &dev_class_queue);
	}

	//add the specified device to the class dev list
	assert(pdev != NULL);
	list_add_tail(&pdev->lcls, &new->dev_queue);
	return 0;
}

static struct device_s* dev_class_get(const char *name)
{
	struct list_head *pos;
	struct class_s *q, *new = NULL;
	struct device_s *pdev = NULL;
	int n, index = 0;

	assert(name != NULL);
	if(strchr(name, '.'))
		return NULL;

	//uart0 -> "uart" + "0", note: if index is not specified, 0 is assumed
	//^_^ "uart_stm32" = "uart_stm" + "32"
	n = strlen(name);
	while(isdigit(name[n - 1]) && (n > 0)) {
		n --;
	}
	index = atoi(name + n);

	//try to find the class with specified name
	list_for_each(pos, &dev_class_queue) {
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
	list_for_each(pos, &new->dev_queue) {
		if(n == index) {
			pdev = list_entry(pos, device_s, lcls);
			break;
		}
	}

	return pdev;
}

