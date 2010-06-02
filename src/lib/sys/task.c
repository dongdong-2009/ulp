/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "sys/task.h"
#include "sys/system.h"

static void (*task_foreground)(void);

void task_Init(void)
{
	void (**init)(void);
	void (**end)(void);
	
	task_foreground = 0;
	
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
	if(task_foreground)
		task_foreground();
	else
		task_update();
}

void task_Isr(void)
{
	if(task_foreground)
		task_update();
}

void task_SetForeground(void (*task)(void))
{
	task_foreground = task;
}
