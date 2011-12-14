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

int drv_register(const char *name, const struct drv_ops_s *ops)
{
	struct driver_s *new;

	//verify name is a correct driver name, like uart_stm32
	assert(strchr(name, '.') == NULL);
	assert(ops != NULL);
	new = sys_malloc(sizeof(struct driver_s));
	assert(new != NULL);

	new->name = name;
	new->ops = ops;
	list_add_tail(&new->list, &drv_queue);
	dev_probe();
	return 0;
}

//when name = "uart_sam3u.debug", driver "uart_sam3u" will be opened
struct driver_s* drv_open(const char *name)
{
	struct list_head *pos;
	struct driver_s *q;
	char *p = strchr(name, '.');
	int n = (p == NULL) ? strlen(name) : (p - name);

	list_for_each(pos, &drv_queue) {
		q = list_entry(pos, driver_s, list);
		if(!strncmp(name, q->name, n) && (n == strlen(q->name))) {
			return q;
		}
	}
	return NULL;
}

int drv_close(struct driver_s *pdrv)
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
