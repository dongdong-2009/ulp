/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "sys/task.h"
#include "sys/system.h"

void task_Init(void)
{
	void (**init)(void);
	void (**end)(void);
	
	sys_Init();
	init = __section_begin(".sys.task");
	end = __section_end(".sys.task");
	while(init < end) {
		(*init)();
		init ++;
		init ++;
	}
}

static void task_update(void)
{
	void (**update)(void);
	void (**end)(void);
	
	sys_Update();
	update = __section_begin(".sys.task");
	end = __section_end(".sys.task");
	while(update < end) {
		update ++;
		(*update)();
		update ++;
	}
}

void task_Update(void)
{
	task_update();
}
