/*
 * 	miaofng@2010 initial version
 */

#include "config.h"
#include "sys/task.h"
#include "sys/system.h"
#include "time.h"
#include "driver.h"

#ifndef CONFIG_LIB_FREERTOS
static time_t task_timer;
static void (*task_foreground)(void);

void task_Init(void)
{
	void (**init)(void);
	void (**end)(void);
	
	task_timer = 0;
	task_foreground = 0;
	
	sys_Init();
	drv_Init();
	
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
	time_isr();
	if(task_foreground) {
		if(time_left(task_timer) < 0) {
			task_timer = time_get(CONFIG_SYSTEM_UPDATE_MS);
			task_update();
		}
	}
}

void task_SetForeground(void (*task)(void))
{
	task_foreground = task;
}
#endif
