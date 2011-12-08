/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "ulp/driver.h"
#include "ulp/device.h"
#include "sys/malloc.h"
#include "linux/list.h"
#include <stdlib.h>
#include <string.h>

static LIST_HEAD(drv_queue);

struct driver_s* drv_register(const char *name, const struct drv_ops_s *ops)
{
	struct driver_s *new;
	struct device_s *dev;
	new = sys_malloc(sizeof(struct drv_s));
	assert(new != NULL);

	new->name = name;
	new->ops = ops;
	list_add_tail(&new->list, &drv_queue);

	//device exist?
	dev = __dev_get(name);
	if(dev != NULL) {

	}

	return 0;
}

struct driver_s* drv_open(const char *name)
{
	struct list_head *pos;
	struct driver_s *q, *drv = NULL;

	list_for_each(pos, &drv_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(!strcmp(name, q->name)) {
			drv = q;
			break;
		}
	}
	return drv;
}

int drv_close(driver_s *pdrv)
{
	return 0;
}

void drv_Init(void)
{
	void (**init)(void);
	void (**end)(void);

	init = __section_begin(".driver");
	end = __section_end(".driver");
	while(init < end) {
		(*init)();
		init ++;
	}
}