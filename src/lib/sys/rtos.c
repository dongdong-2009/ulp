/*
 * 	miaofng@2010 initial version
		this file is used when rtos is enabled
 */

#include "config.h"
#include "sys/task.h"
#include "sys/system.h"
#include "time.h"
#include "driver.h"

/* rtos includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern void xPortSysTickHandler( void );

static void task_init(void)
{
	void (**init)(void);
	void (**end)(void);
	
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

static void vUpdateTask(void *pvParameters)
{
	drv_Init();
	task_init();
	while(1) {
		task_update();
	}
}

void task_Init(void)
{
	sys_Init();
	xTaskCreate( vUpdateTask, (signed portCHAR *) "Update", 256, NULL, tskIDLE_PRIORITY + 1, NULL );	
	
	/* Start the scheduler. */
	vTaskStartScheduler();
	
	/* Will only get here if there was not enough heap space to create the
	idle task. */
}

void task_Update(void)
{
	//never reach here
}

void task_Isr(void)
{
	time_isr();
	xPortSysTickHandler();
}

void task_SetForeground(void (*task)(void))
{
}
