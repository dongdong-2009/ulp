/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "ulp/device.h"
#include "ulp/err.h"

static struct list_head ulp_dev_list;

int dev_register(const char *name, struct device *pdev)
{
	if(name == NULL || name[0] == 0)
		return - ERR_PARA_IN;
	
	pdev->name = name;
	//add dev to ulp device list
	
	//try to find the driver for it
	return 0;
}

int dev_open(const char *path, const char *mode)
{
	//search the ulp_dev_list
	//call its open method
	return 0;
}

int dev_ioctl(int handle, int request, ...)
{
}

int dev_read(void *ptr, int size, int count, int handle)
{
}

int dev_write(const void *ptr, int size, int count, int handle)
{
}

int dev_close(int handle)
{
}


